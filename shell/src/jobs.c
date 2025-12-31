#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static Job jobs[MAX_JOBS];
static int next_job_id = 1;
static pid_t foreground_pid = -1;

void init_jobs(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].in_use = false;
    }
}

int add_job(pid_t pid, const char *command, JobStatus status) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].in_use) {
            jobs[i].job_id = next_job_id++;
            jobs[i].pid = pid;
            strncpy(jobs[i].command, command, MAX_CMD_LEN - 1);
            jobs[i].command[MAX_CMD_LEN - 1] = '\0';
            jobs[i].status = status;
            jobs[i].in_use = true;
            return jobs[i].job_id;
        }
    }
    return -1;
}

Job* get_job(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use && jobs[i].job_id == job_id) {
            return &jobs[i];
        }
    }
    return NULL;
}

Job* get_job_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use && jobs[i].pid == pid) {
            return &jobs[i];
        }
    }
    return NULL;
}

void remove_job(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use && jobs[i].job_id == job_id) {
            jobs[i].in_use = false;
            return;
        }
    }
}

void check_background_jobs(void) {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        Job *job = get_job_by_pid(pid);
        if (job == NULL) {
            continue;
        }
        
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == 0) {
                printf("%s with pid %d exited normally\n", job->command, pid);
            } else {
                printf("%s with pid %d exited abnormally\n", job->command, pid);
            }
            remove_job(job->job_id);
        } else if (WIFSIGNALED(status)) {
            printf("%s with pid %d exited abnormally\n", job->command, pid);
            remove_job(job->job_id);
        } else if (WIFSTOPPED(status)) {
            job->status = JOB_STOPPED;
        } else if (WIFCONTINUED(status)) {
            job->status = JOB_RUNNING;
        }
    }
}

void list_jobs(void) {
    // First, collect all active jobs
    Job *active_jobs[MAX_JOBS];
    int active_count = 0;
    
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use) {
            active_jobs[active_count++] = &jobs[i];
        }
    }
    
    // Sort lexicographically by command name
    for (int i = 0; i < active_count - 1; i++) {
        for (int j = i + 1; j < active_count; j++) {
            if (strcmp(active_jobs[i]->command, active_jobs[j]->command) > 0) {
                Job *temp = active_jobs[i];
                active_jobs[i] = active_jobs[j];
                active_jobs[j] = temp;
            }
        }
    }
    
    // Print sorted jobs
    for (int i = 0; i < active_count; i++) {
        const char *status_str = (active_jobs[i]->status == JOB_RUNNING) ? "Running" : "Stopped";
        printf("[%d] : %s - %s\n", active_jobs[i]->pid, active_jobs[i]->command, status_str);
    }
}

int get_most_recent_job_id(void) {
    int max_id = -1;
    int max_job_id = -1;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].in_use && jobs[i].job_id > max_id) {
            max_id = jobs[i].job_id;
            max_job_id = jobs[i].job_id;
        }
    }
    return max_job_id;
}

void set_foreground_pid(pid_t pid) {
    foreground_pid = pid;
}

pid_t get_foreground_pid(void) {
    return foreground_pid;
}
