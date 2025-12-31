#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

// Token types
typedef enum {
    TOKEN_NAME,
    TOKEN_PIPE,
    TOKEN_SEMICOLON,
    TOKEN_AMPERSAND,
    TOKEN_INPUT,
    TOKEN_OUTPUT,
    TOKEN_APPEND,
    TOKEN_END
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char *value;
    int position;
} Token;

// Parsed command structure
typedef struct Command {
    char **args;
    int arg_count;
    char *input_file;
    char *output_file;
    bool append;
    struct Command *next;
    bool background;
} Command;

typedef struct CommandGroup {
    Command *commands;
    struct CommandGroup *next;
    bool background;
} CommandGroup;

typedef struct {
    CommandGroup *groups;
    bool background;
} ShellCommand;

// Parser functions
bool parse_input(const char *input, ShellCommand **cmd);
void free_shell_command(ShellCommand *cmd);
int tokenize_and_execute(const char *input);

#endif // PARSER_H
