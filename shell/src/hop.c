#include "intrinsics.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

// Implementation of hop command
// Syntax: hop ((~ | . | .. | - | name)*)?
int cmd_hop(int argc, char **argv) {
    char new_dir[PATH_MAX];
    
    // If no arguments, go to home directory
    if (argc == 1) {
        if (chdir(home_dir) != 0) {
            perror("hop");
            return -1;
        }
        // Update prev_dir
        if (getcwd(new_dir, sizeof(new_dir)) != NULL) {
            strcpy(prev_dir, new_dir);
        }
        return 0;
    }
    
    // Process each argument sequentially
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        char target[PATH_MAX];
        
        if (strcmp(arg, "~") == 0) {
            // Go to home directory
            strcpy(target, home_dir);
        } else if (strcmp(arg, ".") == 0) {
            // Stay in current directory (do nothing)
            continue;
        } else if (strcmp(arg, "..") == 0) {
            // Go to parent directory
            strcpy(target, "..");
        } else if (strcmp(arg, "-") == 0) {
            // Go to previous directory
            if (prev_dir[0] == '\0') {
                // No previous directory
                continue;
            }
            strcpy(target, prev_dir);
        } else {
            // Regular path (relative or absolute)
            strcpy(target, arg);
        }
        
        // Save current directory before changing
        char old_dir[PATH_MAX];
        if (getcwd(old_dir, sizeof(old_dir)) == NULL) {
            perror("getcwd");
            return -1;
        }
        
        // Try to change directory
        if (chdir(target) != 0) {
            printf("No such directory!\n");
            return -1;
        }
        
        // Update prev_dir to old directory
        strcpy(prev_dir, old_dir);
    }
    
    return 0;
}
