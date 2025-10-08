#include "redirection.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

// Parse command arguments and extract redirection information
RedirectionInfo parse_redirection(vector<char *> &args)
{
    RedirectionInfo redir;
    vector<char *> clean_args;

    for (size_t i = 0; i < args.size() && args[i] != nullptr; i++)
    {
        string arg = args[i];

        if (arg == "<")
        {
            // Input redirection
            if (i + 1 < args.size() && args[i + 1] != nullptr)
            {
                redir.has_input_redirect = true;
                redir.input_file = args[i + 1];
                i++; // Skip the filename
            }
            else
            {
                cerr << "shell: syntax error near unexpected token '<'" << endl;
                redir = RedirectionInfo(); // Reset to empty
                return redir;
            }
        }
        else if (arg == ">")
        {
            // Output redirection (overwrite)
            if (i + 1 < args.size() && args[i + 1] != nullptr)
            {
                redir.has_output_redirect = true;
                redir.output_append = false;
                redir.output_file = args[i + 1];
                i++; // Skip the filename
            }
            else
            {
                cerr << "shell: syntax error near unexpected token '>'" << endl;
                redir = RedirectionInfo(); // Reset to empty
                return redir;
            }
        }
        else if (arg == ">>")
        {
            // Output redirection (append)
            if (i + 1 < args.size() && args[i + 1] != nullptr)
            {
                redir.has_output_redirect = true;
                redir.output_append = true;
                redir.output_file = args[i + 1];
                i++; // Skip the filename
            }
            else
            {
                cerr << "shell: syntax error near unexpected token '>>'" << endl;
                redir = RedirectionInfo(); // Reset to empty
                return redir;
            }
        }
        else
        {
            // Regular argument
            clean_args.push_back(args[i]);
        }
    }

    // Add null terminator for execvp
    clean_args.push_back(nullptr);
    redir.clean_args = clean_args;

    return redir;
}

// Setup file redirection before executing command
bool setup_redirection(const RedirectionInfo &redir)
{
    // Handle input redirection
    if (redir.has_input_redirect)
    {
        int input_fd = open(redir.input_file.c_str(), O_RDONLY);
        if (input_fd == -1)
        {
            perror(("shell: " + redir.input_file).c_str());
            return false;
        }

        if (dup2(input_fd, STDIN_FILENO) == -1)
        {
            perror("dup2 input");
            close(input_fd);
            return false;
        }
        close(input_fd);
    }

    // Handle output redirection
    if (redir.has_output_redirect)
    {
        int flags;
        if (redir.output_append)
        {
            flags = O_WRONLY | O_CREAT | O_APPEND;
        }
        else
        {
            flags = O_WRONLY | O_CREAT | O_TRUNC;
        }

        int output_fd = open(redir.output_file.c_str(), flags, 0644);
        if (output_fd == -1)
        {
            perror(("shell: " + redir.output_file).c_str());
            return false;
        }

        if (dup2(output_fd, STDOUT_FILENO) == -1)
        {
            perror("dup2 output");
            close(output_fd);
            return false;
        }
        close(output_fd);
    }

    return true;
}

// Restore stdin and stdout
void restore_stdio(int saved_stdin, int saved_stdout)
{
    if (saved_stdin != -1)
    {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }

    if (saved_stdout != -1)
    {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
}