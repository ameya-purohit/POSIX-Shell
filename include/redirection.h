#ifndef REDIRECTION_H
#define REDIRECTION_H

#include <string>
#include <vector>

using namespace std;

// Structure to hold redirection information
struct RedirectionInfo
{
    bool has_input_redirect = false;
    bool has_output_redirect = false;
    bool output_append = false; // true for >>, false for >
    string input_file;
    string output_file;
    vector<char *> clean_args; // Arguments without redirection operators
};

// Function declarations
RedirectionInfo parse_redirection(vector<char *> &args);
bool setup_redirection(const RedirectionInfo &redir);
void restore_stdio(int saved_stdin, int saved_stdout);

#endif