#include "shell.h"
#include "builtins.h"
#include "autocomplete.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <limits.h>

using namespace std;

// External declaration for shell home directory
extern string shell_home_dir;

void sigint_handler(int sig)
{
    (void)sig;
    if (foreground_pid > 0)
    {
        // Kill foreground process and show newline
        kill(foreground_pid, SIGINT);
        write(STDOUT_FILENO, "\n", 1);
    }

    else
    {
        // At shell prompt - just show newline and redisplay
        write(STDOUT_FILENO, "\n", 1);
        rl_on_new_line();
        rl_replace_line("", 0);
        rl_redisplay();
    }
}

void sigtstp_handler(int sig)
{
    (void)sig;
    if (foreground_pid > 0)
    {
        // Suspend foreground process
        kill(foreground_pid, SIGTSTP);
        char msg[100];
        snprintf(msg, sizeof(msg), "\n[Process %d suspended]\n", foreground_pid);
        write(STDOUT_FILENO, msg, strlen(msg));

        // Reset foreground_pid since process is now suspended (background)
        foreground_pid = -1;

        // Redisplay prompt
        rl_on_new_line();
        rl_replace_line("", 0);
        rl_redisplay();
    }

    else
    {
        // At shell prompt - just show newline and redisplay
        write(STDOUT_FILENO, "\n", 1);
        rl_on_new_line();
        rl_replace_line("", 0);
        rl_redisplay();
    }
}

void sigchld_handler(int sig)
{
    (void)sig;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (pid == foreground_pid)
        {
            // Foreground process finished - don't print anything
            foreground_pid = -1;
        }
        else
        {
            // Background process finished - only print if no foreground process
            if (foreground_pid == -1 && !WIFSIGNALED(status))
            {
                char msg[100];
                snprintf(msg, sizeof(msg), "\n[Background process %d finished]\n", pid);
                write(STDOUT_FILENO, msg, strlen(msg));
                rl_on_new_line();
                rl_redisplay();
            }
        }
    }
}

void setup_signal_handlers()
{
    struct sigaction sa_int, sa_tstp, sa_chld;

    // SIGINT handler - Ctrl+C
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0; // No SA_RESTART for cleaner exit behavior
    sigaction(SIGINT, &sa_int, nullptr);

    // SIGTSTP handler - Ctrl+Z
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, nullptr);

    // SIGCHLD handler - Child process status changes
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, nullptr);

    // Ignore SIGPIPE to handle broken pipes gracefully
    signal(SIGPIPE, SIG_IGN);
}

void parse_semicolon_commands(char *input)
{
    if (input == nullptr || strlen(input) == 0)
        return;

    // Created a copy to work with
    string input_str(input);

    // Check for semicolons inside quotes and temporarily replace them
    bool in_quotes = false;
    char quote_char = '\0';
    vector<size_t> protected_semicolons;

    for (size_t i = 0; i < input_str.length(); i++)
    {
        if (!in_quotes && (input_str[i] == '"' || input_str[i] == '\''))
        {
            in_quotes = true;
            quote_char = input_str[i];
        }
        else if (in_quotes && input_str[i] == quote_char)
        {
            in_quotes = false;
            quote_char = '\0';
        }
        else if (in_quotes && input_str[i] == ';')
        {
            protected_semicolons.push_back(i);
            input_str[i] = '\1'; // Temporary placeholder
        }
    }

    // Split on semicolons
    vector<string> commands;
    size_t start = 0;
    size_t pos = 0;

    while ((pos = input_str.find(';', start)) != string::npos)
    {
        if (pos > start)
        {
            commands.push_back(input_str.substr(start, pos - start));
        }
        start = pos + 1;
    }

    // Add the last command
    if (start < input_str.length())
    {
        commands.push_back(input_str.substr(start));
    }

    // Execute each command
    for (auto &cmd : commands)
    {
        // Restore protected semicolons
        for (size_t pos : protected_semicolons)
        {
            if (pos < cmd.length())
            {
                cmd[pos] = ';';
            }
        }

        // Trim whitespace
        size_t first = cmd.find_first_not_of(" \t\n");
        if (first == string::npos)
            continue;

        size_t last = cmd.find_last_not_of(" \t\n");
        cmd = cmd.substr(first, last - first + 1);

        if (!cmd.empty())
        {
            // Created a mutable copy for parse_and_execute
            vector<char> cmd_buffer(cmd.begin(), cmd.end());
            cmd_buffer.push_back('\0');
            parse_and_execute(cmd_buffer.data());
        }
    }
}

int main()
{
    // Initialized shell's home directory to current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)))
    {
        shell_home_dir = string(cwd);
    }

    else
    {
        perror("getcwd");
        shell_home_dir = "/"; // Fallback
    }

    setup_signal_handlers();
    setup_autocomplete(); // Initialized autocomplete functionality
    read_history(".shell_history");

    cout << "Welcome to Ameya's Custom Shell! Type 'exit' to quit.\n";

    while (true)
    {
        string prompt = get_prompt();
        char *input = readline(prompt.c_str());

        // Handle Ctrl+D (EOF)
        if (input == nullptr)
        {
            cout << "Goodbye!\n";
            break;
        }

        // Handle empty input
        if (strlen(input) == 0)
        {
            free(input);
            continue;
        }

        add_history(input);

        // Process semicolon-separated commands
        parse_semicolon_commands(input);

        free(input);
    }

    write_history(".shell_history");
    return 0;
}