#include "shell.h"
#include "builtins.h"
#include "pipeline.h"
#include "redirection.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>

using namespace std;

pid_t foreground_pid = -1;
string shell_home_dir = ""; // Global variable to store shell's starting directory

string get_prompt() // generates a dynamic prompt string user_name@system_name:current_directory>
{
    struct passwd *pw = getpwuid(getuid());
    string username = pw ? pw->pw_name : "user";

    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        perror("gethostname");
        strcpy(hostname, "host");
    }

    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("getcwd");
        strcpy(cwd, "?");
    }

    string dir(cwd);

    // Replace shell's home directory with ~ only if we're inside the shell home tree
    if (!shell_home_dir.empty())
    {
        if (dir == shell_home_dir)
        {
            dir = "~"; // Exactly in shell's home directory
        }
        else if (dir.find(shell_home_dir + "/") == 0)
        {
            dir.replace(0, shell_home_dir.length(), "~"); // Subdirectory of shell home
        }

        // If outside shell home tree, we show full absolute path (no change to dir)
    }

    return username + "@" + hostname + ":" + dir + "> ";
}

vector<char *> tokenize_with_redirection(char *command)
{
    vector<char *> tokens;
    vector<string> temp_tokens; // To store tokens as strings first
    char *start = command;

    while (*start)
    {
        // Skip whitespace
        while (*start && (*start == ' ' || *start == '\t' || *start == '\n'))
            start++;

        if (*start == '\0')
            break;

        // Check for redirection operators
        if (*start == '>' && *(start + 1) == '>')
        {
            // >> operator
            temp_tokens.push_back(">>");
            start += 2;
        }
        else if (*start == '<' || *start == '>')
        {
            // < or > operator
            temp_tokens.push_back(string(1, *start));
            start++;
        }
        else if (*start == '|')
        {
            // | operator
            temp_tokens.push_back("|");
            start++;
        }
        else
        {
            // Regular token
            char *token_start = start;

            // Find end of token
            while (*start && *start != ' ' && *start != '\t' && *start != '\n' &&
                   *start != '<' && *start != '>' && *start != '|')
                start++;

            // Add token
            if (start > token_start)
            {
                temp_tokens.push_back(string(token_start, start - token_start));
            }
        }
    }

    // Convert back to char* vector
    static vector<string> stored_tokens;
    stored_tokens = temp_tokens;

    for (auto &token : stored_tokens)
    {
        tokens.push_back(const_cast<char *>(token.c_str()));
    }

    return tokens;
}

vector<char *> tokenize_simple(char *command)
{
    // Check if command contains redirection operators
    if (strstr(command, "<") || strstr(command, ">") || strstr(command, "|"))
    {
        return tokenize_with_redirection(command);
    }
    else
    {
        vector<char *> tokens;
        char *token = strtok(command, " \t\n");

        while (token != nullptr)
        {
            tokens.push_back(token);
            token = strtok(nullptr, " \t\n");
        }
        return tokens;
    }
}

void execute_command(vector<char *> &args, bool background)
{
    // Parse redirection before executing
    RedirectionInfo redir = parse_redirection(args);

    if (redir.clean_args.empty() || redir.clean_args[0] == nullptr)
    {
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return;
    }
    else if (pid == 0)
    {
        // Child process - restore default signal handlers
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        // Setup redirection in child process
        if (!setup_redirection(redir))
        {
            exit(EXIT_FAILURE);
        }

        if (execvp(redir.clean_args[0], redir.clean_args.data()) == -1)
        {
            if (errno == ENOENT)
            {
                cerr << redir.clean_args[0] << ": command not found" << endl;
            }
            else
            {
                perror("execvp");
            }
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (background)
        {
            cout << "Background process started with PID: " << pid << endl;
        }
        else
        {
            foreground_pid = pid;
            int status;
            if (waitpid(pid, &status, 0) == -1)
            {
                if (errno != EINTR)
                {
                    perror("waitpid");
                }
            }
            foreground_pid = -1;
        }
    }
}

void parse_and_execute(char *command_line)
{
    // Skip leading whitespace
    while (*command_line == ' ' || *command_line == '\t')
        command_line++;

    if (strlen(command_line) == 0)
        return;

    bool background = false;
    int len = strlen(command_line);

    bool in_quotes = false;
    char quote_char = '\0';
    int amp_pos = -1;

    for (int i = 0; i < len; i++)
    {
        if (!in_quotes && (command_line[i] == '"' || command_line[i] == '\''))
        {
            in_quotes = true;
            quote_char = command_line[i];
        }
        else if (in_quotes && command_line[i] == quote_char)
        {
            in_quotes = false;
        }
        else if (!in_quotes && command_line[i] == '&')
        {
            amp_pos = i;
        }
    }

    if (amp_pos != -1)
    {
        bool is_trailing_amp = true;
        for (int i = amp_pos + 1; i < len; i++)
        {
            if (command_line[i] != ' ' && command_line[i] != '\t' && command_line[i] != '\n')
            {
                is_trailing_amp = false;
                break;
            }
        }

        if (is_trailing_amp)
        {
            background = true;
            command_line[amp_pos] = '\0';
            len = amp_pos;
            // Remove trailing whitespace before the &
            while (len > 0 && (command_line[len - 1] == ' ' || command_line[len - 1] == '\t'))
            {
                command_line[len - 1] = '\0';
                len--;
            }
        }
    }

    // Make a copy for tokenization
    string command_copy(command_line);
    vector<char> command_buffer(command_copy.begin(), command_copy.end());
    command_buffer.push_back('\0'); // Null terminate

    vector<char *> tokens = tokenize_simple(command_buffer.data());

    if (tokens.empty())
    {
        return;
    }

    string cmd(tokens[0]);

    // Check for pipelines
    bool has_pipe = false;
    for (char *token : tokens)
    {
        if (token && string(token) == "|")
        {
            has_pipe = true;
            break;
        }
    }

    if (has_pipe)
    {
        // Add pipeline command to history
        string full_cmd = "";
        for (size_t i = 0; i < tokens.size() && tokens[i] != nullptr; i++)
        {
            if (i > 0)
                full_cmd += " ";
            full_cmd += tokens[i];
        }
        add_to_history(full_cmd);
        
        // Parse and execute pipeline
        Pipeline pipeline = parse_pipeline(tokens);
        pipeline.background = background;
        execute_pipeline(pipeline);
        return;
    }

    // Check for builtins that don't support background
    if (background && (cmd == "cd" || cmd == "pwd" || cmd == "echo" || cmd == "ls" || cmd == "pinfo" || cmd == "search" || cmd == "history"))
    {
        cerr << "Background execution not supported for built-in commands\n";
        return;
    }

    // Check if it's a builtin
    if (cmd == "cd" || cmd == "pwd" || cmd == "echo" || cmd == "ls" || cmd == "exit" || cmd == "pinfo" || cmd == "search" || cmd == "history")
    {
        RedirectionInfo redir = parse_redirection(tokens);

        if (redir.clean_args.empty() || redir.clean_args[0] == nullptr)
        {
            return;
        }

        // Save original stdin/stdout for built-ins
        int saved_stdin = -1, saved_stdout = -1;
        if (redir.has_input_redirect || redir.has_output_redirect)
        {
            saved_stdin = dup(STDIN_FILENO);
            saved_stdout = dup(STDOUT_FILENO);

            if (!setup_redirection(redir))
            {
                // Restore if redirection failed
                restore_stdio(saved_stdin, saved_stdout);
                return;
            }
        }

        // Execute builtin with clean arguments
        handle_builtin(redir.clean_args);

        // Restore original stdin/stdout
        restore_stdio(saved_stdin, saved_stdout);
        return;
    }

    // External command
    execute_command(tokens, background);
}