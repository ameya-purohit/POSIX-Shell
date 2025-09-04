#include "autocomplete.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <algorithm>
#include <readline/readline.h>
#include <readline/history.h>

using namespace std;

// Built-in commands for autocomplete
static const vector<string> builtin_commands = {
    "cd", "pwd", "echo", "ls", "exit", "pinfo", "search", "history"};

// Cache for PATH executables to avoid repeated filesystem access
static vector<string> path_executables_cache;
static bool cache_initialized = false;

// Function to get all executables in PATH
vector<string> get_path_executables()
{
    if (cache_initialized)
    {
        return path_executables_cache;
    }

    vector<string> executables;
    const char *path_env = getenv("PATH");
    if (!path_env)
    {
        cache_initialized = true;
        return executables;
    }

    string path_str(path_env);
    size_t start = 0;
    size_t pos = 0;

    // Split PATH by colons
    while ((pos = path_str.find(':', start)) != string::npos)
    {
        string dir = path_str.substr(start, pos - start);
        if (!dir.empty())
        {
            DIR *d = opendir(dir.c_str());
            if (d)
            {
                struct dirent *entry;
                while ((entry = readdir(d)) != nullptr)
                {
                    if (entry->d_name[0] != '.') // Skip hidden files
                    {
                        string full_path = dir + "/" + string(entry->d_name);
                        struct stat st;
                        if (stat(full_path.c_str(), &st) == 0 &&
                            (st.st_mode & S_IXUSR)) // Check if executable
                        {
                            executables.push_back(string(entry->d_name));
                        }
                    }
                }
                closedir(d);
            }
        }
        start = pos + 1;
    }

    // Handle the last directory in PATH
    if (start < path_str.length())
    {
        string dir = path_str.substr(start);
        if (!dir.empty())
        {
            DIR *d = opendir(dir.c_str());
            if (d)
            {
                struct dirent *entry;
                while ((entry = readdir(d)) != nullptr)
                {
                    if (entry->d_name[0] != '.')
                    {
                        string full_path = dir + "/" + string(entry->d_name);
                        struct stat st;
                        if (stat(full_path.c_str(), &st) == 0 &&
                            (st.st_mode & S_IXUSR))
                        {
                            executables.push_back(string(entry->d_name));
                        }
                    }
                }
                closedir(d);
            }
        }
    }

    // Remove duplicates
    sort(executables.begin(), executables.end());
    executables.erase(unique(executables.begin(), executables.end()), executables.end());

    path_executables_cache = executables;
    cache_initialized = true;
    return executables;
}

// Function to get files and directories in current directory
vector<string> get_current_directory_entries()
{
    vector<string> entries;
    DIR *d = opendir(".");
    if (!d)
        return entries;

    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr)
    {
        string name(entry->d_name);
        if (name != "." && name != "..") // Skip . and ..
        {
            entries.push_back(name);
        }
    }
    closedir(d);

    sort(entries.begin(), entries.end());
    return entries;
}

// Function to check if we're completing a command (first word) or file/directory
bool is_command_completion(const char *line_buffer, int start)
{
    // Skip leading whitespace
    int i = 0;
    while (i < start && (line_buffer[i] == ' ' || line_buffer[i] == '\t'))
        i++;

    // Check if we're at the beginning or only whitespace before
    for (int j = i; j < start; j++)
    {
        if (line_buffer[j] != ' ' && line_buffer[j] != '\t')
            return false; // There's already a command, so this is file completion
    }

    return true; // this is command completion
}

// Generator function for command completion
char *command_name_generator(const char *text, int state)
{
    static vector<string> all_commands;
    static size_t list_index;
    static size_t text_len;

    if (state == 0) // First call
    {
        all_commands.clear();
        list_index = 0;
        text_len = strlen(text);

        // Add built-in commands
        for (const auto &cmd : builtin_commands)
        {
            all_commands.push_back(cmd);
        }

        // Add PATH executables
        vector<string> path_execs = get_path_executables();
        all_commands.insert(all_commands.end(), path_execs.begin(), path_execs.end());

        // Sort and remove duplicates
        sort(all_commands.begin(), all_commands.end());
        all_commands.erase(unique(all_commands.begin(), all_commands.end()), all_commands.end());
    }

    // Return matching commands
    while (list_index < all_commands.size())
    {
        const string &cmd = all_commands[list_index++];
        if (cmd.length() >= text_len && cmd.substr(0, text_len) == text)
        {
            return strdup(cmd.c_str());
        }
    }

    return nullptr;
}

// Generator function for file/directory completion
char *filename_generator(const char *text, int state)
{
    static vector<string> entries;
    static size_t list_index;
    static size_t text_len;

    if (state == 0) // First call
    {
        entries = get_current_directory_entries();
        list_index = 0;
        text_len = strlen(text);
    }

    // Return matching files/directories
    while (list_index < entries.size())
    {
        const string &entry = entries[list_index++];
        if (entry.length() >= text_len && entry.substr(0, text_len) == text)
        {
            return strdup(entry.c_str());
        }
    }

    return nullptr;
}

// Main completion function
char **shell_completion(const char *text, int start, int end)
{
    (void)end; // Unused parameter
    char **matches = nullptr;

    // Determine if we're completing a command or filename
    if (is_command_completion(rl_line_buffer, start))
    {
        // Command completion
        matches = rl_completion_matches(text, command_name_generator);
    }
    else
    {
        // File/directory completion
        matches = rl_completion_matches(text, filename_generator);
    }

    return matches;
}

// Initialize autocomplete
void setup_autocomplete()
{
    // Setting our custom completion function
    rl_attempted_completion_function = shell_completion;

    // Setting the word break characters (characters that separate words)
    // Including |, <, > as word separators for proper pipeline and redirection completion
    rl_completer_word_break_characters = (char *)" \t\n\"\\'`@$><=;|&{(";

    // Don't append space after completion - let readline handle it naturally
    rl_completion_append_character = '\0';

    // Don't use filename completion as the default fallback
    rl_completion_entry_function = nullptr;

    // Initialize the PATH executables cache
    get_path_executables();
}