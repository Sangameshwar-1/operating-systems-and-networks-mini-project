#include "shell.h"
#include "jobs.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sigint_handler(int sig) {
    (void)sig;
    pid_t fg_pid = get_foreground_pid();
    if (fg_pid > 0) {
        kill(fg_pid, SIGINT);
    }
    printf("\n");
}

void sigtstp_handler(int sig) {
    (void)sig;
    pid_t fg_pid = get_foreground_pid();
    if (fg_pid > 0) {
        kill(fg_pid, SIGTSTP);
        Job *job = get_job_by_pid(fg_pid);
        if (job) {
            job->status = JOB_STOPPED;
            printf("\n[%d] Stopped %s\n", job->job_id, job->command);
        }
        set_foreground_pid(-1);
    }
}

void setup_signal_handlers(void) {
    struct sigaction sa_int, sa_tstp;
    
    // SIGINT (Ctrl-C)
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);
    
    // SIGTSTP (Ctrl-Z)
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);
}
