#include "intrinsics.h"
#include "shell.h"
#include "executor.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>

#define MAX_LOG_ENTRIES 15
#define LOG_FILE ".shell_log"

// Global log state
static char log_entries[MAX_LOG_ENTRIES][MAX_INPUT_LEN];
static int log_count = 0;
static int log_start = 0; // Ring buffer start index

// Load log from file
static void load_log(void) {
    char log_path[PATH_MAX + 20];  // Extra space for filename
    int ret = snprintf(log_path, sizeof(log_path), "%s/%s", home_dir, LOG_FILE);
    if (ret < 0 || (size_t)ret >= sizeof(log_path)) {
        return; // Path too long
    }
    
    FILE *f = fopen(log_path, "r");
    if (f == NULL) {
        return; // No log file yet
    }
    
    log_count = 0;
    log_start = 0;
    
    char line[MAX_INPUT_LEN];
    while (fgets(line, sizeof(line), f) != NULL && log_count < MAX_LOG_ENTRIES) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        strcpy(log_entries[log_count], line);
        log_count++;
    }
    
    fclose(f);
}

// Save log to file
static void save_log(void) {
    char log_path[PATH_MAX + 20];  // Extra space for filename
    int ret = snprintf(log_path, sizeof(log_path), "%s/%s", home_dir, LOG_FILE);
    if (ret < 0 || (size_t)ret >= sizeof(log_path)) {
        return; // Path too long
    }
    
    FILE *f = fopen(log_path, "w");
    if (f == NULL) {
        perror("log: failed to save");
        return;
    }
    
    for (int i = 0; i < log_count; i++) {
        int idx = (log_start + i) % MAX_LOG_ENTRIES;
        fprintf(f, "%s\n", log_entries[idx]);
    }
    
    fclose(f);
}

// Global flag for log loading
static bool log_loaded = false;

// Add command to log
void log_add_command(const char *cmd) {
    // Load log on first use
    if (!log_loaded) {
        load_log();
        log_loaded = true;
    }
    
    // Don't log if command contains "log" as the first command name
    // Simple check: if the command starts with "log"
    const char *trimmed = cmd;
    while (*trimmed == ' ' || *trimmed == '\t') {
        trimmed++;
    }
    
    if (strncmp(trimmed, "log", 3) == 0 &&
        (trimmed[3] == '\0' || trimmed[3] == ' ' || trimmed[3] == '\t')) {
        return;
    }
    
    // Check if identical to previous command
    if (log_count > 0) {
        int last_idx = (log_start + log_count - 1) % MAX_LOG_ENTRIES;
        if (strcmp(log_entries[last_idx], cmd) == 0) {
            return; // Don't add duplicate
        }
    }
    
    // Add to ring buffer
    if (log_count < MAX_LOG_ENTRIES) {
        strcpy(log_entries[log_count], cmd);
        log_count++;
    } else {
        // Overwrite oldest
        strcpy(log_entries[log_start], cmd);
        log_start = (log_start + 1) % MAX_LOG_ENTRIES;
    }
    
    save_log();
}

// Implementation of log command
// Syntax: log (purge | execute <index>)?
int cmd_log(int argc, char **argv) {
    // Load log on first use
    if (!log_loaded) {
        load_log();
        log_loaded = true;
    }
    
    if (argc == 1) {
        // Print all log entries (oldest to newest)
        for (int i = 0; i < log_count; i++) {
            int idx = (log_start + i) % MAX_LOG_ENTRIES;
            printf("%s\n", log_entries[idx]);
        }
    } else if (argc == 2 && strcmp(argv[1], "purge") == 0) {
        // Clear log
        log_count = 0;
        log_start = 0;
        save_log();
    } else if (argc == 3 && strcmp(argv[1], "execute") == 0) {
        // Execute command at index (1-indexed, newest to oldest)
        int index = atoi(argv[2]);
        if (index < 1 || index > log_count) {
            printf("log: Invalid index\n");
            return -1;
        }
        
        // Convert to array index (newest to oldest)
        int idx = (log_start + log_count - index) % MAX_LOG_ENTRIES;
        
        // Execute the command without logging it
        // We parse and execute directly
        ShellCommand *cmd = NULL;
        if (!parse_input(log_entries[idx], &cmd)) {
            printf("Invalid Syntax!\n");
            return -1;
        }
        
        execute_full_command(log_entries[idx]);
        free_shell_command(cmd);
        return 0;
    } else {
        printf("log: Invalid Syntax!\n");
        return -1;
    }
    
    return 0;
}
