# xv6 Modifications Patch Guide

This file contains all the modifications needed to implement getreadcount syscall, FCFS, CFS, and MLFQ schedulers in xv6.

## Step 1: Apply Syscall Changes

### 1.1 kernel/syscall.h
Add at the end (before #endif):
```c
#define SYS_getreadcount 23
```

### 1.2 kernel/syscall.c
Add to the extern declarations section:
```c
extern uint64 sys_getreadcount(void);
```

Add to the syscalls array:
```c
[SYS_getreadcount] sys_getreadcount,
```

### 1.3 kernel/sysproc.c
Add at the top with other includes:
```c
extern uint64 total_bytes_read;
```

Add at the end of the file:
```c
uint64
sys_getreadcount(void)
{
  return total_bytes_read;
}
```

### 1.4 kernel/sysfile.c (or wherever sys_read is)
Add at the top with global variables:
```c
uint64 total_bytes_read = 0;
```

Find the sys_read function and after successful read, add:
```c
if(n > 0)
  total_bytes_read += n;
```

### 1.5 user/user.h
Add to the system call declarations:
```c
int getreadcount(void);
```

### 1.6 user/usys.pl (this generates usys.S)
Add:
```perl
entry("getreadcount");
```

## Step 2: Add Scheduler Fields to proc structure

### 2.1 kernel/proc.h
Add to struct proc:
```c
  uint64 ctime;           // Creation time (for FCFS)
  int nice;               // Nice value for CFS
  uint64 vruntime;        // Virtual runtime for CFS
  uint64 weight;          // Weight for CFS
  uint64 runtime;         // Actual runtime for CFS
  int queue;              // Queue level for MLFQ
  int time_slice;         // Time slice for MLFQ
  uint64 queue_enter_time; // When entered queue for MLFQ
```

### 2.2 kernel/proc.c - allocproc()
Find allocproc() and add initialization after p->pid = allocpid():
```c
  p->ctime = ticks;
  p->nice = 0;
  p->vruntime = 0;
  p->weight = 1024;
  p->runtime = 0;
  p->queue = 0;
  p->time_slice = 1;
  p->queue_enter_time = ticks;
```

## Step 3: Implement Schedulers

### 3.1 Modify Makefile
Find the CFLAGS line and add:
```makefile
ifndef SCHEDULER
SCHEDULER := RR
endif

CFLAGS += -DSCHEDULER_$(SCHEDULER)
```

### 3.2 kernel/proc.c - scheduler() function
Replace the entire scheduler() function with:

```c
#ifdef SCHEDULER_FCFS
// FCFS Scheduler
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    intr_on();
    
    struct proc *earliest = 0;
    for(p = proc; p < &proc[NPROC]; p++){
      acquire(&p->lock);
      if(p->state == RUNNABLE){
        if(earliest == 0 || p->ctime < earliest->ctime){
          if(earliest)
            release(&earliest->lock);
          earliest = p;
        } else {
          release(&p->lock);
        }
      } else {
        release(&p->lock);
      }
    }
    
    if(earliest){
      earliest->state = RUNNING;
      c->proc = earliest;
      swtch(&c->context, &earliest->context);
      c->proc = 0;
      release(&earliest->lock);
    }
  }
}

#elif defined(SCHEDULER_CFS)
// CFS Scheduler with logging

static uint64
calculate_weight(int nice)
{
  if(nice == 0) return 1024;
  uint64 weight = 1024;
  if(nice < 0) {
    for(int i = 0; i < -nice; i++)
      weight = (weight * 5) / 4;
    return weight;
  } else {
    for(int i = 0; i < nice; i++)
      weight = (weight * 4) / 5;
    return weight < 15 ? 15 : weight;
  }
}

void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    intr_on();
    
    struct proc *min_proc = 0;
    uint64 min_vruntime = -1ULL;
    int runnable_count = 0;
    
    // Count runnable processes
    for(p = proc; p < &proc[NPROC]; p++){
      acquire(&p->lock);
      if(p->state == RUNNABLE)
        runnable_count++;
      release(&p->lock);
    }
    
    // Log scheduling decision
    printf("[Scheduler Tick]\n");
    for(p = proc; p < &proc[NPROC]; p++){
      acquire(&p->lock);
      if(p->state == RUNNABLE){
        printf("PID: %d | vRuntime: %d\n", p->pid, p->vruntime);
        if(min_proc == 0 || p->vruntime < min_vruntime){
          if(min_proc)
            release(&min_proc->lock);
          min_proc = p;
          min_vruntime = p->vruntime;
        } else {
          release(&p->lock);
        }
      } else {
        release(&p->lock);
      }
    }
    
    if(min_proc){
      printf("--> Scheduling PID %d (lowest vRuntime)\n", min_proc->pid);
      
      min_proc->state = RUNNING;
      c->proc = min_proc;
      
      uint64 start_runtime = min_proc->runtime;
      swtch(&c->context, &min_proc->context);
      
      uint64 elapsed = min_proc->runtime - start_runtime;
      min_proc->vruntime += (elapsed * 1024) / min_proc->weight;
      
      c->proc = 0;
      release(&min_proc->lock);
    }
  }
}

#elif defined(SCHEDULER_MLFQ)
// MLFQ Scheduler (Bonus)

#define QUEUE_COUNT 4
#define BOOST_INTERVAL 48
static int queue_time_slices[QUEUE_COUNT] = {1, 4, 8, 16};
static uint64 last_boost = 0;

void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    intr_on();
    
    // Priority boost every 48 ticks
    if(ticks - last_boost >= BOOST_INTERVAL) {
      for(p = proc; p < &proc[NPROC]; p++){
        acquire(&p->lock);
        if(p->state == RUNNABLE || p->state == RUNNING) {
          p->queue = 0;
          p->time_slice = queue_time_slices[0];
        }
        release(&p->lock);
      }
      last_boost = ticks;
    }
    
    // Find highest priority queue with runnable process
    struct proc *selected = 0;
    for(int q = 0; q < QUEUE_COUNT; q++){
      for(p = proc; p < &proc[NPROC]; p++){
        acquire(&p->lock);
        if(p->state == RUNNABLE && p->queue == q){
          if(selected)
            release(&selected->lock);
          selected = p;
          break;
        }
        release(&p->lock);
      }
      if(selected) break;
    }
    
    if(selected){
      selected->state = RUNNING;
      c->proc = selected;
      
      uint64 start_ticks = ticks;
      swtch(&c->context, &selected->context);
      uint64 elapsed = ticks - start_ticks;
      
      selected->time_slice -= elapsed;
      if(selected->time_slice <= 0){
        if(selected->queue < QUEUE_COUNT - 1)
          selected->queue++;
        selected->time_slice = queue_time_slices[selected->queue];
      }
      
      c->proc = 0;
      release(&selected->lock);
    }
  }
}

#else
// Default Round Robin (keep original xv6 scheduler)
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    intr_on();
    
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}
#endif
```

### 3.3 kernel/trap.c - usertrap()
Find usertrap() and add runtime tracking. After `struct proc *p = myproc();` add:
```c
#ifdef SCHEDULER_CFS
  if(p != 0)
    p->runtime++;
#endif
```

### 3.4 Add readcount to Makefile UPROGS
Find the UPROGS section and add:
```makefile
	$U/_readcount\
```

## Testing Instructions

### Build and test default (Round Robin):
```bash
make qemu
```

### Build and test FCFS:
```bash
make clean
make qemu SCHEDULER=FCFS
```

### Build and test CFS:
```bash
make clean
make qemu SCHEDULER=CFS
```

### Build and test MLFQ (bonus):
```bash
make clean
make qemu SCHEDULER=MLFQ
```

### Test getreadcount:
In xv6 shell:
```
$ readcount
```

### View process info:
Press Ctrl-P in xv6 to see process information

## Verification Checklist
- [ ] getreadcount returns 0 initially
- [ ] getreadcount increases after reading files
- [ ] FCFS schedules oldest process first
- [ ] CFS logs show vruntime values and scheduling decisions
- [ ] MLFQ uses different time slices for different queues
- [ ] All schedulers compile without errors
