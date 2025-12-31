#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

// Execute a parsed command
int execute_command(ShellCommand *cmd);

// Execute a simple command (for intrinsics)
int execute_simple_command(char **args, int arg_count);

// Execute full command with pipes, redirections, etc.
int execute_full_command(const char *input);

#endif // EXECUTOR_H
