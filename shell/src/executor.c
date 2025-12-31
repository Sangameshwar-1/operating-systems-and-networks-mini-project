#include "executor.h"
#include "intrinsics.h"
#include "shell.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_ARGS 256
#define MAX_PIPES 100

typedef struct {
    char **args;
    int arg_count;
    char *input_file;
    char *output_file;
    bool append_output;
} ParsedCommand;

// Check if command is an intrinsic
static bool is_intrinsic(const char *cmd) {
    return strcmp(cmd, "hop") == 0 ||
           strcmp(cmd, "reveal") == 0 ||
           strcmp(cmd, "log") == 0 ||
           strcmp(cmd, "activities") == 0 ||
           strcmp(cmd, "ping") == 0 ||
           strcmp(cmd, "fg") == 0 ||
           strcmp(cmd, "bg") == 0;
}

// Execute intrinsic command
static int execute_intrinsic(char **args, int arg_count) {
    if (strcmp(args[0], "hop") == 0) {
        return cmd_hop(arg_count, args);
    } else if (strcmp(args[0], "reveal") == 0) {
        return cmd_reveal(arg_count, args);
    } else if (strcmp(args[0], "log") == 0) {
        return cmd_log(arg_count, args);
    } else if (strcmp(args[0], "activities") == 0) {
        return cmd_activities(arg_count, args);
    } else if (strcmp(args[0], "ping") == 0) {
        return cmd_ping(arg_count, args);
    } else if (strcmp(args[0], "fg") == 0) {
        return cmd_fg(arg_count, args);
    } else if (strcmp(args[0], "bg") == 0) {
        return cmd_bg(arg_count, args);
    }
    return -1;
}

// Free parsed command
static void free_parsed_command(ParsedCommand *cmd) {
    if (cmd->args) {
        for (int i = 0; i < cmd->arg_count; i++) {
            free(cmd->args[i]);
        }
        free(cmd->args);
    }
    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
}

// Parse a single command with redirections
static int parse_command(const char *input, ParsedCommand *cmd) {
    cmd->args = malloc(MAX_ARGS * sizeof(char*));
    cmd->arg_count = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_output = false;
    
    char buffer[4096];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char *ptr = buffer;
    
    while (*ptr) {
        // Skip whitespace
        while (*ptr && isspace(*ptr)) ptr++;
        if (!*ptr) break;
        
        // Check for redirections
        if (*ptr == '<') {
            ptr++;
            while (*ptr && isspace(*ptr)) ptr++;
            char *start = ptr;
            while (*ptr && !isspace(*ptr) && *ptr != '>' && *ptr != '<') ptr++;
            if (ptr > start) {
                cmd->input_file = strndup(start, ptr - start);
            }
        } else if (*ptr == '>') {
            ptr++;
            if (*ptr == '>') {
                cmd->append_output = true;
                ptr++;
            }
            while (*ptr && isspace(*ptr)) ptr++;
            char *start = ptr;
            while (*ptr && !isspace(*ptr) && *ptr != '>' && *ptr != '<') ptr++;
            if (ptr > start) {
                if (cmd->output_file) free(cmd->output_file);
                cmd->output_file = strndup(start, ptr - start);
            }
        } else {
            // Regular argument
            char *start = ptr;
            while (*ptr && !isspace(*ptr) && *ptr != '>' && *ptr != '<') ptr++;
            if (ptr > start) {
                cmd->args[cmd->arg_count++] = strndup(start, ptr - start);
            }
        }
    }
    
    cmd->args[cmd->arg_count] = NULL;
    return cmd->arg_count > 0 ? 0 : -1;
}

// Execute single command with redirections
static int execute_single_command(ParsedCommand *cmd, bool background) {
    if (cmd->arg_count == 0) return 0;
    
    // Handle intrinsics (can't be backgrounded with redirections in this simple impl)
    if (is_intrinsic(cmd->args[0]) && !cmd->input_file && !cmd->output_file) {
        return execute_intrinsic(cmd->args, cmd->arg_count);
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        
        // Handle input redirection
        if (cmd->input_file) {
            int fd = open(cmd->input_file, O_RDONLY);
            if (fd < 0) {
                perror("No such file or directory");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        
        // Handle output redirection
        if (cmd->output_file) {
            int flags = O_WRONLY | O_CREAT;
            flags |= cmd->append_output ? O_APPEND : O_TRUNC;
            int fd = open(cmd->output_file, flags, 0644);
            if (fd < 0) {
                perror("Unable to create file for writing");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        
        // Redirect stdin from /dev/null for background processes
        if (background) {
            int fd = open("/dev/null", O_RDONLY);
            if (fd >= 0) {
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
        }
        
        execvp(cmd->args[0], cmd->args);
        printf("Command not found!\n");
        exit(1);
    } else if (pid > 0) {
        if (background) {
            // Background process
            int job_id = add_job(pid, cmd->args[0], JOB_RUNNING);
            printf("[%d] %d\n", job_id, pid);
        } else {
            // Foreground process
            set_foreground_pid(pid);
            int status;
            waitpid(pid, &status, WUNTRACED);
            set_foreground_pid(-1);
            
            if (WIFSTOPPED(status)) {
                int job_id = add_job(pid, cmd->args[0], JOB_STOPPED);
                printf("[%d] Stopped %s\n", job_id, cmd->args[0]);
            }
        }
        return 0;
    } else {
        perror("fork");
        return -1;
    }
}

// Execute pipeline
static int execute_pipeline(const char *input, bool background) {
    // Split by pipes
    char buffer[4096];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char *commands[MAX_PIPES];
    int cmd_count = 0;
    
    char *start = buffer;
    char *ptr = buffer;
    
    while (*ptr) {
        if (*ptr == '|') {
            *ptr = '\0';
            commands[cmd_count++] = start;
            start = ptr + 1;
        }
        ptr++;
    }
    commands[cmd_count++] = start;
    
    if (cmd_count == 1) {
        // No pipes, simple command
        ParsedCommand cmd;
        if (parse_command(commands[0], &cmd) == 0) {
            int ret = execute_single_command(&cmd, background);
            free_parsed_command(&cmd);
            return ret;
        }
        return -1;
    }
    
    // Multiple commands in pipeline
    int pipes[MAX_PIPES][2];
    pid_t pids[MAX_PIPES];
    
    // Create pipes
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return -1;
        }
    }
    
    // Fork and execute each command
    for (int i = 0; i < cmd_count; i++) {
        ParsedCommand cmd;
        if (parse_command(commands[i], &cmd) != 0) {
            free_parsed_command(&cmd);
            continue;
        }
        
        pids[i] = fork();
        if (pids[i] == 0) {
            // Child process
            
            // Redirect stdin from previous pipe
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Redirect stdout to next pipe
            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe fds
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Handle file redirections
            if (cmd.input_file && i == 0) {
                int fd = open(cmd.input_file, O_RDONLY);
                if (fd < 0) {
                    perror("No such file or directory");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            
            if (cmd.output_file && i == cmd_count - 1) {
                int flags = O_WRONLY | O_CREAT;
                flags |= cmd.append_output ? O_APPEND : O_TRUNC;
                int fd = open(cmd.output_file, flags, 0644);
                if (fd < 0) {
                    perror("Unable to create file for writing");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            
            execvp(cmd.args[0], cmd.args);
            printf("Command not found!\n");
            exit(1);
        }
        
        free_parsed_command(&cmd);
    }
    
    // Close all pipe fds in parent
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children
    if (background) {
        // Add last process as background job
        ParsedCommand last_cmd;
        parse_command(commands[cmd_count - 1], &last_cmd);
        int job_id = add_job(pids[cmd_count - 1], last_cmd.args[0], JOB_RUNNING);
        printf("[%d] %d\n", job_id, pids[cmd_count - 1]);
        free_parsed_command(&last_cmd);
    } else {
        for (int i = 0; i < cmd_count; i++) {
            int status;
            waitpid(pids[i], &status, 0);
        }
    }
    
    return 0;
}

// Execute a simple command (for basic usage)
int execute_simple_command(char **args, int arg_count) {
    if (arg_count == 0 || args[0] == NULL) {
        return 0;
    }
    
    if (is_intrinsic(args[0])) {
        return execute_intrinsic(args, arg_count);
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        printf("Command not found!\n");
        exit(1);
    } else if (pid > 0) {
        set_foreground_pid(pid);
        int status;
        waitpid(pid, &status, WUNTRACED);
        set_foreground_pid(-1);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    } else {
        perror("fork");
        return -1;
    }
}

// Main execution function
int execute_full_command(const char *input) {
    // Parse for ; and & operators
    char buffer[4096];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char *cmd_groups[100];
    bool backgrounds[100];
    int group_count = 0;
    
    char *start = buffer;
    char *ptr = buffer;
    
    while (*ptr) {
        if (*ptr == ';' || *ptr == '&') {
            char sep = *ptr;
            *ptr = '\0';
            
            // Trim whitespace
            char *end = ptr - 1;
            while (end >= start && isspace(*end)) {
                *end = '\0';
                end--;
            }
            
            if (start < ptr && *start != '\0') {
                cmd_groups[group_count] = start;
                backgrounds[group_count] = (sep == '&');
                group_count++;
            }
            
            start = ptr + 1;
            while (*start && isspace(*start)) start++;
        }
        ptr++;
    }
    
    // Handle last command or trailing &
    if (start < ptr) {
        char *end = ptr - 1;
        while (end >= start && isspace(*end)) {
            *end = '\0';
            end--;
        }
        if (*start != '\0') {
            cmd_groups[group_count] = start;
            backgrounds[group_count] = false;
            group_count++;
        }
    }
    
    // Execute each command group
    for (int i = 0; i < group_count; i++) {
        execute_pipeline(cmd_groups[i], backgrounds[i]);
    }
    
    return 0;
}

// Execute a parsed command
int execute_command(ShellCommand *cmd) {
    (void)cmd;
    return 0;
}
