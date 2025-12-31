#include "shell.h"
#include "parser.h"
#include "intrinsics.h"
#include "executor.h"
#include "jobs.h"
#include <signal.h>

// Global variables
char home_dir[PATH_MAX];
char prev_dir[PATH_MAX];

void init_shell(void) {
    // Get current working directory as home directory
    if (getcwd(home_dir, sizeof(home_dir)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    
    // Initialize prev_dir to empty
    prev_dir[0] = '\0';
    
    // Initialize job management
    init_jobs();
    
    // Setup signal handlers
    setup_signal_handlers();
}

void display_prompt(void) {
    char hostname[256];
    char cwd[PATH_MAX];
    char display_path[PATH_MAX];
    struct passwd *pw;
    
    // Get username
    pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("getpwuid");
        return;
    }
    
    // Get hostname
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        return;
    }
    
    // Get current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }
    
    // Replace home directory with ~
    size_t home_len = strlen(home_dir);
    if (strncmp(cwd, home_dir, home_len) == 0) {
        if (cwd[home_len] == '\0') {
            // cwd is exactly home_dir
            snprintf(display_path, sizeof(display_path), "~");
        } else if (cwd[home_len] == '/') {
            // cwd is a subdirectory of home_dir
            snprintf(display_path, sizeof(display_path), "~%s", cwd + home_len);
        } else {
            // cwd starts with home_dir but is not a subdirectory
            snprintf(display_path, sizeof(display_path), "%s", cwd);
        }
    } else {
        // cwd does not have home_dir as ancestor
        snprintf(display_path, sizeof(display_path), "%s", cwd);
    }
    
    printf("<%s@%s:%s> ", pw->pw_name, hostname, display_path);
    fflush(stdout);
}

int process_input(const char *input) {
    ShellCommand *cmd = NULL;
    
    // Parse the input
    if (!parse_input(input, &cmd)) {
        printf("Invalid Syntax!\n");
        return -1;
    }
    
    // Execute the full command with all features
    execute_full_command(input);
    
    free_shell_command(cmd);
    return 0;
}

int main(void) {
    char input[MAX_INPUT_LEN];
    
    // Initialize shell
    init_shell();
    
    // Main shell loop
    while (1) {
        // Check for completed background jobs
        check_background_jobs();
        
        display_prompt();
        
        // Read input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // EOF (Ctrl-D) or error
            if (feof(stdin)) {
                printf("logout\n");
                // Kill all child processes
                signal(SIGTERM, SIG_IGN);
                kill(0, SIGKILL);
                break;
            }
            continue;
        }
        
        // Remove trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        
        // Skip leading whitespace
        char *trimmed = input;
        while (*trimmed == ' ' || *trimmed == '\t') {
            trimmed++;
        }
        
        // Skip empty input or comments
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }
        
        // Add to log before processing
        log_add_command(trimmed);
        
        // Process the input
        process_input(trimmed);
    }
    
    return 0;
}
