#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <vector>

using namespace std;

// Global variables
extern pid_t foreground_pid;
extern string shell_home_dir;

// Function declarations
string get_prompt();
vector<char *> tokenize_simple(char *command);
vector<char *> tokenize_with_redirection(char *command);
void execute_command(vector<char *> &args, bool background);
void parse_and_execute(char *command_line);
void parse_semicolon_commands(char *input);

#endif