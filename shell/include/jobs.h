#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <stdbool.h>

#define MAX_JOBS 100
#define MAX_CMD_LEN 1024

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobStatus;

typedef struct {
    int job_id;
    pid_t pid;
    char command[MAX_CMD_LEN];
    JobStatus status;
    bool in_use;
} Job;

// Job management functions
void init_jobs(void);
int add_job(pid_t pid, const char *command, JobStatus status);
Job* get_job(int job_id);
Job* get_job_by_pid(pid_t pid);
void remove_job(int job_id);
void check_background_jobs(void);
void list_jobs(void);
int get_most_recent_job_id(void);

// Foreground process management
void set_foreground_pid(pid_t pid);
pid_t get_foreground_pid(void);

#endif // JOBS_H
