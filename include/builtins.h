#ifndef BUILTINS_H
#define BUILTINS_H

#include <vector>
#include <string>

// Function declarations for built-in commands
void builtin_ls(const std::vector<std::string> &args);
int builtin_cd(std::vector<char *> args);
int builtin_pwd(std::vector<char *> args);
int builtin_echo(std::vector<char *> args);
void add_to_history(const string &command);
bool handle_builtin(std::vector<char *> args);

#endif