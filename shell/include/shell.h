#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>

// Maximum input length
#define MAX_INPUT_LEN 4096

// Global shell state
extern char home_dir[PATH_MAX];
extern char prev_dir[PATH_MAX];

// Function prototypes
void display_prompt(void);
void init_shell(void);
int process_input(const char *input);
void setup_signal_handlers(void);

#endif // SHELL_H
