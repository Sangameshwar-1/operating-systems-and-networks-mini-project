#include "intrinsics.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <stdbool.h>

// Comparison function for qsort
static int compare_entries(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// Implementation of reveal command
// Syntax: reveal (-(a | l)*)* (~ | . |.. | - | name)?
int cmd_reveal(int argc, char **argv) {
    bool show_hidden = false;
    bool long_format = false;
    char target_dir[PATH_MAX];
    int arg_count = 0;
    
    // Parse flags and arguments
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            // Parse flags
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'a') {
                    show_hidden = true;
                } else if (argv[i][j] == 'l') {
                    long_format = true;
                } else {
                    printf("reveal: Invalid flag -%c\n", argv[i][j]);
                    return -1;
                }
            }
        } else {
            // It's a directory argument
            arg_count++;
            if (arg_count > 1) {
                printf("reveal: Invalid Syntax!\n");
                return -1;
            }
            
            // Resolve the target directory
            if (strcmp(argv[i], "~") == 0) {
                strcpy(target_dir, home_dir);
            } else if (strcmp(argv[i], ".") == 0) {
                if (getcwd(target_dir, sizeof(target_dir)) == NULL) {
                    perror("getcwd");
                    return -1;
                }
            } else if (strcmp(argv[i], "..") == 0) {
                strcpy(target_dir, "..");
            } else if (strcmp(argv[i], "-") == 0) {
                if (prev_dir[0] == '\0') {
                    printf("No such directory!\n");
                    return -1;
                }
                strcpy(target_dir, prev_dir);
            } else {
                strcpy(target_dir, argv[i]);
            }
        }
    }
    
    // If no directory specified, use current directory
    if (arg_count == 0) {
        if (getcwd(target_dir, sizeof(target_dir)) == NULL) {
            perror("getcwd");
            return -1;
        }
    }
    
    // Open directory
    DIR *dir = opendir(target_dir);
    if (dir == NULL) {
        printf("No such directory!\n");
        return -1;
    }
    
    // Read all entries
    char **entries = NULL;
    int entry_count = 0;
    int entry_capacity = 32;
    entries = malloc(entry_capacity * sizeof(char *));
    if (entries == NULL) {
        perror("malloc");
        closedir(dir);
        return -1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files if -a not specified
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        // Add entry to list
        if (entry_count >= entry_capacity) {
            entry_capacity *= 2;
            char **new_entries = realloc(entries, entry_capacity * sizeof(char *));
            if (new_entries == NULL) {
                perror("realloc");
                for (int i = 0; i < entry_count; i++) {
                    free(entries[i]);
                }
                free(entries);
                closedir(dir);
                return -1;
            }
            entries = new_entries;
        }
        
        entries[entry_count] = strdup(entry->d_name);
        if (entries[entry_count] == NULL) {
            perror("strdup");
            for (int i = 0; i < entry_count; i++) {
                free(entries[i]);
            }
            free(entries);
            closedir(dir);
            return -1;
        }
        entry_count++;
    }
    
    closedir(dir);
    
    // Sort entries lexicographically
    qsort(entries, entry_count, sizeof(char *), compare_entries);
    
    // Print entries
    if (long_format) {
        // One entry per line
        for (int i = 0; i < entry_count; i++) {
            printf("%s\n", entries[i]);
        }
    } else {
        // Space-separated on one line (like ls)
        for (int i = 0; i < entry_count; i++) {
            if (i > 0) {
                printf(" ");
            }
            printf("%s", entries[i]);
        }
        if (entry_count > 0) {
            printf("\n");
        }
    }
    
    // Free memory
    for (int i = 0; i < entry_count; i++) {
        free(entries[i]);
    }
    free(entries);
    
    return 0;
}
