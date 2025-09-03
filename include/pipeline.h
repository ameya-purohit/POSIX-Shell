#ifndef PIPELINE_H
#define PIPELINE_H

#include <string>
#include <vector>
#include "redirection.h"

using namespace std;

// Structure to hold a single command in a pipeline
struct Command
{
    vector<char *> args;
    RedirectionInfo redirection;
    bool has_redirection = false;
};

// Structure to hold pipeline information
struct Pipeline
{
    vector<Command> commands;
    bool background = false;
};

// Function declarations
Pipeline parse_pipeline(vector<char *> &args);
void execute_pipeline(const Pipeline &pipeline);
bool is_builtin_command(const string &cmd);

#endif