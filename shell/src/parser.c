#include "parser.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ARGS 256

// Simple tokenizer for basic command execution
// This is a simplified version for Part B - will be enhanced for Parts C-E
int tokenize_and_execute(const char *input) {
    char *input_copy = strdup(input);
    if (input_copy == NULL) {
        return -1;
    }
    
    char *args[MAX_ARGS];
    int arg_count = 0;
    
    // Simple whitespace tokenization
    char *token = strtok(input_copy, " \t\n\r");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t\n\r");
    }
    args[arg_count] = NULL;
    
    int result = 0;
    if (arg_count > 0) {
        result = execute_simple_command(args, arg_count);
    }
    
    free(input_copy);
    return result;
}

// Helper function to check if character is a special symbol
static bool is_special_char(char c) {
    return c == '|' || c == '&' || c == '>' || c == '<' || c == ';';
}

// Helper function to skip whitespace
static const char* skip_whitespace(const char *str) {
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

// Helper function to check if we have a valid name character
static bool is_name_char(char c) {
    return c != '\0' && !isspace((unsigned char)c) && !is_special_char(c);
}

// Parse a name token
static const char* parse_name(const char *input, char **name) {
    const char *start = input;
    const char *end = input;
    
    // Read until we hit whitespace or special character
    while (is_name_char(*end)) {
        end++;
    }
    
    if (end == start) {
        return NULL; // No name found
    }
    
    // Allocate and copy name
    size_t len = end - start;
    *name = malloc(len + 1);
    if (*name == NULL) {
        return NULL;
    }
    strncpy(*name, start, len);
    (*name)[len] = '\0';
    
    return end;
}

// Parse input redirection: < name or <name
static const char* parse_input_redir(const char *input, char **filename) {
    if (*input != '<') {
        return NULL;
    }
    input++; // Skip '<'
    
    // Skip optional whitespace
    input = skip_whitespace(input);
    
    // Parse filename
    return parse_name(input, filename);
}

// Parse output redirection: > name or >name or >> name or >>name
static const char* parse_output_redir(const char *input, char **filename, bool *append) {
    if (*input != '>') {
        return NULL;
    }
    input++; // Skip first '>'
    
    // Check for '>>'
    if (*input == '>') {
        *append = true;
        input++; // Skip second '>'
    } else {
        *append = false;
    }
    
    // Skip optional whitespace
    input = skip_whitespace(input);
    
    // Parse filename
    return parse_name(input, filename);
}

// Parse atomic command: name (name | input | output)*
static const char* parse_atomic(const char *input) {
    input = skip_whitespace(input);
    
    // Must start with a name
    char *name = NULL;
    const char *next = parse_name(input, &name);
    if (next == NULL) {
        return NULL;
    }
    free(name); // We're just validating syntax
    input = next;
    
    // Parse arguments and redirections
    while (1) {
        input = skip_whitespace(input);
        
        if (*input == '\0' || *input == '|' || *input == '&' || *input == ';') {
            // End of atomic command
            break;
        }
        
        // Try to parse input redirection
        char *filename = NULL;
        next = parse_input_redir(input, &filename);
        if (next != NULL) {
            free(filename);
            input = next;
            continue;
        }
        
        // Try to parse output redirection
        bool append;
        next = parse_output_redir(input, &filename, &append);
        if (next != NULL) {
            free(filename);
            input = next;
            continue;
        }
        
        // Try to parse a name (argument)
        next = parse_name(input, &filename);
        if (next != NULL) {
            free(filename);
            input = next;
            continue;
        }
        
        // Invalid token
        return NULL;
    }
    
    return input;
}

// Parse command group: atomic (\| atomic)*
static const char* parse_cmd_group(const char *input) {
    input = skip_whitespace(input);
    
    // Must have at least one atomic command
    const char *next = parse_atomic(input);
    if (next == NULL) {
        return NULL;
    }
    input = next;
    
    // Parse additional piped commands
    while (1) {
        input = skip_whitespace(input);
        
        if (*input != '|') {
            break;
        }
        input++; // Skip '|'
        
        input = skip_whitespace(input);
        
        next = parse_atomic(input);
        if (next == NULL) {
            return NULL; // Invalid syntax after pipe
        }
        input = next;
    }
    
    return input;
}

// Parse shell command: cmd_group ((& | ;) cmd_group)* &?
bool parse_input(const char *input, ShellCommand **cmd) {
    input = skip_whitespace(input);
    
    // Empty input is valid
    if (*input == '\0') {
        *cmd = NULL;
        return true;
    }
    
    // Parse first command group
    const char *next = parse_cmd_group(input);
    if (next == NULL) {
        return false;
    }
    input = next;
    
    // Parse additional command groups with ; or &
    while (1) {
        input = skip_whitespace(input);
        
        if (*input == '\0') {
            break;
        }
        
        if (*input == '&') {
            input++; // Skip '&'
            input = skip_whitespace(input);
            
            // Check if there's another command group after &
            if (*input == '\0') {
                // Trailing & is valid
                break;
            }
            
            next = parse_cmd_group(input);
            if (next == NULL) {
                return false;
            }
            input = next;
        } else if (*input == ';') {
            input++; // Skip ';'
            input = skip_whitespace(input);
            
            // Must have another command group after ;
            next = parse_cmd_group(input);
            if (next == NULL) {
                return false;
            }
            input = next;
        } else {
            // Unexpected character
            return false;
        }
    }
    
    input = skip_whitespace(input);
    
    // Should be at end of input
    if (*input != '\0') {
        return false;
    }
    
    // For now, we just validate syntax without building the AST
    *cmd = NULL;
    return true;
}

void free_shell_command(ShellCommand *cmd) {
    // TODO: Implement when we build the actual AST
    if (cmd) {
        free(cmd);
    }
}
