#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include <vector>
#include <string>

using namespace std;

// Function declarations
vector<string> get_path_executables();
vector<string> get_current_directory_entries();
bool is_command_completion(const char *line_buffer, int start);
bool is_after_redirection(const char *line_buffer, int start);
int find_command_start(const char *line_buffer, int start);
char *command_name_generator(const char *text, int state);
char *filename_generator(const char *text, int state);
char **shell_completion(const char *text, int start, int end);
void setup_autocomplete();

#endif