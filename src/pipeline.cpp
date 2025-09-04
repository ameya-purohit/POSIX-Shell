#include "pipeline.h"
#include "redirection.h"
#include "builtins.h"
#include "shell.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

using namespace std;

extern pid_t foreground_pid;

// Check if command is a builtin
bool is_builtin_command(const string &cmd)
{
    return (cmd == "cd" || cmd == "pwd" || cmd == "echo" || cmd == "ls" ||
            cmd == "exit" || cmd == "pinfo" || cmd == "search" || cmd == "history");
}

// Parses pipeline from tokens
Pipeline parse_pipeline(vector<char *> &args)
{
    Pipeline pipeline;
    Command current_command;

    for (size_t i = 0; i < args.size() && args[i] != nullptr; i++)
    {
        string arg = args[i];

        if (arg == "|")
        {
            // End of current command, starting new one
            if (!current_command.args.empty())
            {
                current_command.redirection = parse_redirection(current_command.args);
                current_command.has_redirection = (current_command.redirection.has_input_redirect ||
                                                   current_command.redirection.has_output_redirect);

                if (current_command.has_redirection && !current_command.redirection.clean_args.empty())
                {
                    current_command.args = current_command.redirection.clean_args;
                }

                else
                {
                    // null terminator
                    current_command.args.push_back(nullptr);
                }

                pipeline.commands.push_back(current_command);
            }

            // Reset for next command
            current_command = Command();
        }
        else
        {
            current_command.args.push_back(args[i]);
        }
    }

    // Handling the last command
    if (!current_command.args.empty())
    {
        current_command.redirection = parse_redirection(current_command.args);
        current_command.has_redirection = (current_command.redirection.has_input_redirect ||
                                           current_command.redirection.has_output_redirect);

        if (current_command.has_redirection && !current_command.redirection.clean_args.empty())
        {
            current_command.args = current_command.redirection.clean_args;
        }
        else
        {
            // null terminator
            current_command.args.push_back(nullptr);
        }

        pipeline.commands.push_back(current_command);
    }

    return pipeline;
}

// Executes a single command in the pipeline
pid_t execute_command_in_pipeline(const Command &cmd, int input_fd, int output_fd)
{
    if (cmd.args.empty() || cmd.args[0] == nullptr)
    {
        return -1;
    }

    string command_name = cmd.args[0];

    // Handling builtin commands differently
    if (is_builtin_command(command_name))
    {
        // For builtins in a pipeline, we need to fork to avoid affecting the shell
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);

            // Setup pipe redirection
            if (input_fd != STDIN_FILENO)
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (output_fd != STDOUT_FILENO)
            {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }

            // Setup file redirection if present
            if (cmd.has_redirection && !setup_redirection(cmd.redirection))
            {
                exit(EXIT_FAILURE);
            }

            // Execute builtin
            vector<char *> builtin_args = cmd.args;
            handle_builtin(builtin_args);
            exit(EXIT_SUCCESS);
        }
        else if (pid < 0)
        {
            perror("fork");
        }
        return pid;
    }
    else
    {
        // External command
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);

            // Setup pipe redirection
            if (input_fd != STDIN_FILENO)
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (output_fd != STDOUT_FILENO)
            {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }

            // Setup file redirection if present
            if (cmd.has_redirection && !setup_redirection(cmd.redirection))
            {
                exit(EXIT_FAILURE);
            }

            // Execute external command
            if (execvp(cmd.args[0], cmd.args.data()) == -1)
            {
                if (errno == ENOENT)
                {
                    cerr << cmd.args[0] << ": command not found" << endl;
                }
                else
                {
                    perror("execvp");
                }
                exit(EXIT_FAILURE);
            }
        }
        else if (pid < 0)
        {
            perror("fork");
        }
        return pid;
    }
}

// Executing entire pipeline
void execute_pipeline(const Pipeline &pipeline)
{
    if (pipeline.commands.empty())
    {
        return;
    }

    // Single command - no pipes needed
    if (pipeline.commands.size() == 1)
    {
        const Command &cmd = pipeline.commands[0];

        if (cmd.args.empty() || cmd.args[0] == nullptr)
        {
            return;
        }

        string command_name = cmd.args[0];

        // Handling builtin commands
        if (is_builtin_command(command_name))
        {
            // Save original stdin/stdout for built-ins
            int saved_stdin = -1, saved_stdout = -1;
            if (cmd.has_redirection)
            {
                saved_stdin = dup(STDIN_FILENO);
                saved_stdout = dup(STDOUT_FILENO);

                if (!setup_redirection(cmd.redirection))
                {
                    restore_stdio(saved_stdin, saved_stdout);
                    return;
                }
            }

            // Executing builtin
            vector<char *> builtin_args = cmd.args;
            handle_builtin(builtin_args);

            // Restoring stdio
            restore_stdio(saved_stdin, saved_stdout);
            return;
        }
        else
        {
            // External command - used existing execute_command logic
            vector<char *> external_args = cmd.args;
            if (cmd.has_redirection)
            {
                // Reconstructed args with redirection for execute_command
                vector<char *> full_args;
                for (auto arg : cmd.args)
                {
                    if (arg != nullptr)
                    {
                        full_args.push_back(arg);
                    }
                }

                // Added redirection operators back
                if (cmd.redirection.has_input_redirect)
                {
                    full_args.push_back(const_cast<char *>("<"));
                    full_args.push_back(const_cast<char *>(cmd.redirection.input_file.c_str()));
                }
                if (cmd.redirection.has_output_redirect)
                {
                    if (cmd.redirection.output_append)
                    {
                        full_args.push_back(const_cast<char *>(">>"));
                    }
                    else
                    {
                        full_args.push_back(const_cast<char *>(">"));
                    }
                    full_args.push_back(const_cast<char *>(cmd.redirection.output_file.c_str()));
                }

                execute_command(full_args, pipeline.background);
            }
            else
            {
                execute_command(external_args, pipeline.background);
            }
            return;
        }
    }

    // Multiple commands - setup pipeline
    vector<pid_t> pids;
    vector<int> pipes;

    // Create pipes
    for (size_t i = 0; i < pipeline.commands.size() - 1; i++)
    {
        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            perror("pipe");
            return;
        }
        pipes.push_back(pipefd[0]); // read end
        pipes.push_back(pipefd[1]); // write end
    }

    // Execute each command in the pipeline
    for (size_t i = 0; i < pipeline.commands.size(); i++)
    {
        int input_fd = STDIN_FILENO;
        int output_fd = STDOUT_FILENO;

        // Setup input
        if (i > 0)
        {
            input_fd = pipes[(i - 1) * 2]; // read end of previous pipe
        }

        // Setup output
        if (i < pipeline.commands.size() - 1)
        {
            output_fd = pipes[i * 2 + 1]; // write end of current pipe
        }

        pid_t pid = execute_command_in_pipeline(pipeline.commands[i], input_fd, output_fd);
        if (pid > 0)
        {
            pids.push_back(pid);
        }

        // Close pipe ends in parent
        if (input_fd != STDIN_FILENO)
        {
            close(input_fd);
        }
        if (output_fd != STDOUT_FILENO)
        {
            close(output_fd);
        }
    }

    // Close all remaining pipe descriptors
    for (int fd : pipes)
    {
        close(fd);
    }

    // Wait for all processes
    if (!pipeline.background)
    {
        if (!pids.empty())
        {
            foreground_pid = pids.back(); // Last process in pipeline is foreground
        }

        for (pid_t pid : pids)
        {
            int status;
            if (waitpid(pid, &status, 0) == -1)
            {
                if (errno != EINTR)
                {
                    perror("waitpid");
                }
            }
        }

        foreground_pid = -1;
    }
    else
    {
        cout << "Background pipeline started" << endl;
    }
}