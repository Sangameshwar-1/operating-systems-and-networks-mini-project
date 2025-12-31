#include "intrinsics.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

// Implementation of activities command
int cmd_activities(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    list_jobs();
    return 0;
}

// Implementation of ping command
// Syntax: ping <pid> <signal_number>
int cmd_ping(int argc, char **argv) {
    if (argc != 3) {
        printf("Invalid syntax!\n");
        return -1;
    }
    
    // Parse PID
    char *endptr;
    long pid_long = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || pid_long <= 0) {
        printf("Invalid syntax!\n");
        return -1;
    }
    pid_t pid = (pid_t)pid_long;
    
    // Parse signal number
    long signal_long = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        printf("Invalid syntax!\n");
        return -1;
    }
    
    // Modulo 32
    int signal = (int)(signal_long % 32);
    
    // Send signal
    if (kill(pid, signal) == -1) {
        printf("No such process found\n");
        return -1;
    }
    
    printf("Sent signal %ld to process with pid %d\n", signal_long, pid);
    return 0;
}

// Implementation of fg command
// Syntax: fg [job_number]
int cmd_fg(int argc, char **argv) {
    int job_id;
    
    if (argc == 1) {
        // Use most recent job
        job_id = get_most_recent_job_id();
        if (job_id == -1) {
            printf("No such job\n");
            return -1;
        }
    } else if (argc == 2) {
        char *endptr;
        long job_long = strtol(argv[1], &endptr, 10);
        if (*endptr != '\0' || job_long <= 0) {
            printf("No such job\n");
            return -1;
        }
        job_id = (int)job_long;
    } else {
        printf("fg: Invalid Syntax!\n");
        return -1;
    }
    
    Job *job = get_job(job_id);
    if (job == NULL) {
        printf("No such job\n");
        return -1;
    }
    
    printf("%s\n", job->command);
    
    // Send SIGCONT if stopped
    if (job->status == JOB_STOPPED) {
        kill(job->pid, SIGCONT);
    }
    
    job->status = JOB_RUNNING;
    set_foreground_pid(job->pid);
    
    // Wait for job to complete or stop
    int status;
    waitpid(job->pid, &status, WUNTRACED);
    
    set_foreground_pid(-1);
    
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        remove_job(job_id);
    } else if (WIFSTOPPED(status)) {
        job->status = JOB_STOPPED;
    }
    
    return 0;
}

// Implementation of bg command
// Syntax: bg [job_number]
int cmd_bg(int argc, char **argv) {
    int job_id;
    
    if (argc == 1) {
        // Use most recent job
        job_id = get_most_recent_job_id();
        if (job_id == -1) {
            printf("No such job\n");
            return -1;
        }
    } else if (argc == 2) {
        char *endptr;
        long job_long = strtol(argv[1], &endptr, 10);
        if (*endptr != '\0' || job_long <= 0) {
            printf("No such job\n");
            return -1;
        }
        job_id = (int)job_long;
    } else {
        printf("bg: Invalid Syntax!\n");
        return -1;
    }
    
    Job *job = get_job(job_id);
    if (job == NULL) {
        printf("No such job\n");
        return -1;
    }
    
    if (job->status == JOB_RUNNING) {
        printf("Job already running\n");
        return -1;
    }
    
    // Send SIGCONT
    kill(job->pid, SIGCONT);
    job->status = JOB_RUNNING;
    
    printf("[%d] %s &\n", job_id, job->command);
    
    return 0;
}
