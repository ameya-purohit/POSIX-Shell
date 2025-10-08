#include "shell.h"
#include "builtins.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <unistd.h>
#include <limits.h>
#include <cstdio>
#include <sstream>
#include <cstdlib>

using namespace std;

static string previous_dir = "";

// External declaration from shell.cpp
extern string shell_home_dir;

// History storage
static vector<string> command_history;
static const int MAX_HISTORY = 20;
static const int DEFAULT_DISPLAY = 10;

// Helper function to get user's actual home directory
string get_user_home_dir()
{
    // HOME environment variable
    const char *home = getenv("HOME");
    if (home && strlen(home) > 0)
    {
        return string(home);
    }

    // If HOME is not set, get it from passwd database
    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_dir)
    {
        return string(pw->pw_dir);
    }

    // Fallback to /
    return "/";
}

void add_to_history(const string &command)
{
    if (!command.empty() && command != "history")
    {
        command_history.push_back(command);
        if (command_history.size() > MAX_HISTORY)
        {
            command_history.erase(command_history.begin());
        }
    }
}

string get_permissions(mode_t mode)
{
    string perms(10, '-');
    perms[0] = S_ISDIR(mode) ? 'd' : (S_ISLNK(mode) ? 'l' : '-');
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    return perms;
}

void print_long_format(const string &path, const char *filename)
{
    if (!filename)
    {
        cerr << "Error: null filename in print_long_format\n";
        return;
    }

    string fullpath;
    if (path == "." || path.empty())
    {
        fullpath = filename;
    }
    else
    {
        fullpath = path + "/" + filename;
    }

    struct stat st;
    if (lstat(fullpath.c_str(), &st) == -1)
    {
        perror(("lstat: " + fullpath).c_str());
        return;
    }

    string perms = get_permissions(st.st_mode);
    cout << perms << " " << setw(3) << st.st_nlink << " ";

    struct passwd *pw = getpwuid(st.st_uid);
    cout << setw(8) << (pw ? pw->pw_name : "unknown") << " ";

    struct group *grp = getgrgid(st.st_gid);
    cout << setw(8) << (grp ? grp->gr_name : "unknown") << " ";

    cout << setw(8) << st.st_size << " ";

    char timebuf[80];
    struct tm *tm_info = localtime(&st.st_mtime);
    if (tm_info)
    {
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);
        cout << timebuf << " ";
    }
    else
    {
        cout << "??? ?? ??:?? ";
    }

    cout << filename;

    if (S_ISLNK(st.st_mode))
    {
        char link_target[PATH_MAX];
        ssize_t len = readlink(fullpath.c_str(), link_target, sizeof(link_target) - 1);
        if (len != -1)
        {
            link_target[len] = '\0';
            cout << " -> " << link_target;
        }
    }
    cout << endl;
}

void list_directory(const string &path, bool show_all, bool long_format)
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
    {
        perror(("Cannot open directory: " + path).c_str());
        return;
    }

    struct dirent *entry;
    vector<string> files;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (!show_all && entry->d_name[0] == '.')
        {
            continue;
        }
        files.push_back(string(entry->d_name));
    }
    closedir(dir);

    sort(files.begin(), files.end());
    for (const auto &file : files)
    {
        if (long_format)
        {
            print_long_format(path, file.c_str());
        }
        else
        {
            cout << file << endl;
        }
    }
}

void builtin_ls(const vector<string> &args)
{
    bool show_all = false;
    bool long_format = false;
    vector<string> paths;

    // Parse arguments
    for (size_t i = 1; i < args.size(); i++)
    {
        string arg = args[i];
        if (!arg.empty() && arg[0] == '-')
        {
            // Handle combined flags like -al, -la
            for (size_t j = 1; j < arg.size(); j++)
            {
                if (arg[j] == 'a')
                {
                    show_all = true;
                }
                else if (arg[j] == 'l')
                {
                    long_format = true;
                }
                else
                {
                    cerr << "ls: invalid option -- '" << arg[j] << "'\n";
                    return;
                }
            }
        }
        else
        {
            paths.push_back(arg);
        }
    }

    if (paths.empty())
    {
        paths.push_back(".");
    }

    // Expand ~ in paths
    string user_home = get_user_home_dir();
    for (auto &p : paths)
    {
        if (p == "~")
        {
            p = user_home;
        }
        else if (p.length() > 1 && p[0] == '~' && p[1] == '/')
        {
            p = user_home + p.substr(1);
        }
    }

    bool multi_dirs = paths.size() > 1;
    for (size_t i = 0; i < paths.size(); i++)
    {
        if (multi_dirs)
        {
            cout << paths[i] << ":" << endl;
        }

        struct stat st;
        if (stat(paths[i].c_str(), &st) == -1)
        {
            perror(("ls: cannot access " + paths[i]).c_str());
            continue;
        }

        if (S_ISDIR(st.st_mode))
        {
            list_directory(paths[i], show_all, long_format);
        }
        else
        {
            if (long_format)
            {
                size_t pos = paths[i].find_last_of('/');
                if (pos == string::npos)
                {
                    print_long_format(".", paths[i].c_str());
                }
                else
                {
                    string dir = paths[i].substr(0, pos);
                    string file = paths[i].substr(pos + 1);
                    print_long_format(dir, file.c_str());
                }
            }
            else
            {
                cout << paths[i] << endl;
            }
        }

        if (i + 1 < paths.size())
        {
            cout << endl;
        }
    }
}

int builtin_cd(vector<char *> args)
{
    int argc = 0;
    while (argc < (int)args.size() && args[argc] != nullptr)
        argc++;

    if (argc > 2)
    {
        cerr << "cd: too many arguments\n";
        return -1;
    }

    const char *dir = nullptr;
    string user_home = get_user_home_dir();
    string arg = "";

    if (argc == 1) // if no arguments - go to user's actual home directory
    {
        dir = user_home.c_str();
    }

    else if (argc == 2)
    {
        arg = args[1];

        if (arg == "~") // Tilde - go to user's actual home directory
        {
            dir = user_home.c_str();
        }

        else if (arg.length() > 1 && arg[0] == '~' && arg[1] == '/') // ~/path - relative to user home
        {
            static string expanded_path = user_home + arg.substr(1);
            dir = expanded_path.c_str();
        }

        else if (arg == ".") // Current directory
        {
            // Stay in current directory
            return 0;
        }

        else if (arg == "..") // Parent directory
        {
            dir = "..";
        }

        else if (arg == "-") // Previous directory
        {
            if (previous_dir.empty())
            {
                cerr << "cd: OLDPWD not set\n";
                return -1;
            }

            dir = previous_dir.c_str();
        }

        else if (arg[0] == '-') // Invalid options like -s, -a
        {
            cerr << "cd: " << arg << ": No such file or directory\n";
            return -1;
        }
        else // Regular path
        {
            dir = args[1];
        }
    }

    if (!dir)
    {
        cerr << "cd: invalid directory\n";
        return -1;
    }
    // Store current directory before changing (only if chdir succeeds)
    char current_cwd[PATH_MAX];
    string old_dir = "";
    if (getcwd(current_cwd, sizeof(current_cwd)))
    {
        old_dir = string(current_cwd);
    }

    if (chdir(dir) != 0)
    {
        perror("cd");
        return -1;
    }

    // Only update previous_dir after successful chdir
    if (!old_dir.empty())
    {
        previous_dir = old_dir;
    }

    return 0;
}

int builtin_pwd(vector<char *> args)
{
    (void)args; // pwd ignores arguments

    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("pwd");
        return -1;
    }

    // pwd always prints the absolute path, never with ~ substitution
    cout << cwd << endl;
    return 0;
}

int builtin_echo(vector<char *> args)
{
    int argc = 0;
    while (argc < (int)args.size() && args[argc] != nullptr)
        argc++;

    for (int i = 1; i < argc; i++)
    {
        cout << args[i];
        if (i + 1 < argc)
        {
            cout << " ";
        }
    }
    cout << endl;
    return 0;
}

// Helper function to read file content
string read_file(const string &filename)
{
    FILE *file = fopen(filename.c_str(), "r");
    if (!file)
    {
        return "";
    }
    ostringstream content_stream;
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        content_stream.write(buffer, bytes_read);
    }
    fclose(file);
    return content_stream.str();
}

int builtin_pinfo(vector<char *> args)
{
    int argc = 0;
    while (argc < (int)args.size() && args[argc] != nullptr)
        argc++;

    pid_t pid;
    if (argc == 1)
    {
        pid = getpid(); // Current shell process
    }
    else if (argc == 2)
    {
        pid = atoi(args[1]);
        if (pid <= 0)
        {
            cerr << "pinfo: invalid PID\n";
            return -1;
        }
    }
    else
    {
        cerr << "pinfo: too many arguments\n";
        return -1;
    }

    string stat_file = "/proc/" + to_string(pid) + "/stat";
    string status_file = "/proc/" + to_string(pid) + "/status";
    string exe_file = "/proc/" + to_string(pid) + "/exe";

    // Read stat file
    string stat_content = read_file(stat_file);
    if (stat_content.empty())
    {
        cerr << "pinfo: process " << pid << " not found\n";
        return -1;
    }

    // Parse stat file to get state and memory info
    istringstream stat_stream(stat_content);
    string token;
    vector<string> stat_fields;
    while (stat_stream >> token)
    {
        stat_fields.push_back(token);
    }

    if (stat_fields.size() < 23)
    {
        cerr << "pinfo: could not parse process info\n";
        return -1;
    }

    char state = stat_fields[2][0];               // Process state
    unsigned long vsize = stoul(stat_fields[22]); // Virtual memory size

    // Check if process is in foreground
    bool is_foreground = (pid == foreground_pid);
    string status_str;
    status_str += state;
    if (is_foreground)
    {
        status_str += "+";
    }

    // Get executable path
    char exe_path[PATH_MAX];
    ssize_t len = readlink(exe_file.c_str(), exe_path, sizeof(exe_path) - 1);
    string executable;
    if (len != -1)
    {
        exe_path[len] = '\0';
        executable = exe_path;

        // Convert to relative path if in shell's home directory (for display purposes)
        if (!shell_home_dir.empty() && executable.find(shell_home_dir) == 0)
        {
            executable.replace(0, shell_home_dir.length(), "~");
        }
    }
    else
    {
        executable = "Unknown";
    }

    // Display process info
    cout << "Process Status -- " << status_str << endl;
    cout << "memory -- " << vsize << " {Virtual Memory}" << endl;
    cout << "Executable Path -- " << executable << endl;

    return 0;
}

// Helper function for recursive search
bool search_recursive(const string &path, const string &target, bool ignore_hidden = true)
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
    {
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        string name = entry->d_name;

        // Skip . and ..
        if (name == "." || name == "..")
        {
            continue;
        }

        // Skip hidden files if requested
        if (ignore_hidden && name[0] == '.')
        {
            continue;
        }

        // Check if this is the target
        if (name == target)
        {
            closedir(dir);
            return true;
        }

        // If it's a directory, search recursively
        string full_path = path + "/" + name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        {
            if (search_recursive(full_path, target, ignore_hidden))
            {
                closedir(dir);
                return true;
            }
        }
    }

    closedir(dir);
    return false;
}

int builtin_search(vector<char *> args)
{
    int argc = 0;
    while (argc < (int)args.size() && args[argc] != nullptr)
        argc++;

    if (argc != 2)
    {
        cerr << "search: usage: search <filename>\n";
        return -1;
    }

    string target = args[1];
    bool found = search_recursive(".", target, true); // Ignore hidden files

    cout << (found ? "True" : "False") << endl;
    return 0;
}

int builtin_history(vector<char *> args)
{
    int argc = 0;
    while (argc < (int)args.size() && args[argc] != nullptr)
        argc++;

    int num_to_show = DEFAULT_DISPLAY;

    if (argc > 2)
    {
        cerr << "history: too many arguments\n";
        return -1;
    }

    if (argc == 2)
    {
        num_to_show = atoi(args[1]);
        if (num_to_show <= 0)
        {
            cerr << "history: invalid number\n";
            return -1;
        }
        if (num_to_show > MAX_HISTORY)
        {
            num_to_show = MAX_HISTORY;
        }
    }

    int start_idx = max(0, (int)command_history.size() - num_to_show);
    for (int i = start_idx; i < (int)command_history.size(); i++)
    {
        cout << i + 1 << "  " << command_history[i] << endl;
    }

    return 0;
}

bool handle_builtin(vector<char *> args)
{
    if (args.empty())
    {
        return true;
    }

    if (args[0] == nullptr)
    {
        return true;
    }

    string cmd = args[0];

    // Add command to history (except history command itself)
    if (cmd != "history" && args.size() > 0)
    {
        string full_cmd = cmd;
        for (size_t i = 1; i < args.size() && args[i] != nullptr; i++)
        {
            full_cmd += " " + string(args[i]);
        }
        add_to_history(full_cmd);
    }

    if (cmd == "cd")
    {
        return builtin_cd(args) == 0;
    }
    if (cmd == "pwd")
    {
        return builtin_pwd(args) == 0;
    }
    if (cmd == "echo")
    {
        return builtin_echo(args) == 0;
    }
    if (cmd == "ls")
    {
        vector<string> str_args;
        for (size_t i = 0; i < args.size() && args[i] != nullptr; ++i)
        {
            str_args.push_back(string(args[i]));
        }
        builtin_ls(str_args);
        return true;
    }
    if (cmd == "pinfo")
    {
        return builtin_pinfo(args) == 0;
    }
    if (cmd == "search")
    {
        return builtin_search(args) == 0;
    }
    if (cmd == "history")
    {
        return builtin_history(args) == 0;
    }
    if (cmd == "exit")
    {
        exit(0);
    }

    return false;
}