# xv6 Operating System Modifications

Custom implementations of system calls and process schedulers for the xv6 teaching operating system (RISC-V version).

---

## Overview

This project extends xv6 with:
1. **Custom System Calls** - `getreadcount`, `waitx`, `set_priority`
2. **Process Schedulers** - FCFS, CFS, MLFQ implementations
3. **Performance Analysis** - Comparative scheduler evaluation

---

## Features Implemented

### 1. System Calls

#### `getreadcount()`
Returns the total number of bytes read from files since boot.
```c
int count = getreadcount();
printf("Total bytes read: %d\n", count);
```

#### `waitx(int *wtime, int *rtime)`
Extended wait that returns:
- `wtime`: Waiting time (in ticks)
- `rtime`: Running time (in ticks)
```c
int wtime, rtime;
int pid = waitx(&wtime, &rtime);
printf("PID %d: wait=%d, run=%d\n", pid, wtime, rtime);
```

#### `set_priority(int priority, int pid)`
Set process priority (for priority-based schedulers):
- Lower number = Higher priority
- Range: 0-100
```c
set_priority(10, getpid());  // Set high priority
```

### 2. Schedulers

#### Round Robin (RR) - Default
- Time quantum: 1 timer tick
- Fair time-sharing among processes
- Default xv6 scheduler

#### First-Come-First-Serve (FCFS)
- Non-preemptive scheduler
- Processes run to completion
- No time quantum
- Order based on creation time

#### Completely Fair Scheduler (CFS)
- Inspired by Linux CFS
- Virtual runtime (vruntime) tracking
- Processes with least vruntime run first
- Fair CPU time distribution

#### Multi-Level Feedback Queue (MLFQ)
- 5 priority queues (0=highest, 4=lowest)
- Time quantum increases with priority level
- Aging mechanism prevents starvation
- Dynamic priority adjustment

---

## Building and Running

### Prerequisites
```bash
# Install RISC-V toolchain
sudo apt-get update
sudo apt-get install gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
sudo apt-get install qemu-system-misc

# Verify installation
riscv64-linux-gnu-gcc --version
qemu-system-riscv64 --version
```

### Build with Different Schedulers

**Round Robin (Default):**
```bash
cd xv6-source
make clean
make
make qemu
```

**FCFS Scheduler:**
```bash
make clean
make SCHEDULER=FCFS
make qemu
```

**CFS Scheduler:**
```bash
make clean
make SCHEDULER=CFS
make qemu
```

**MLFQ Scheduler:**
```bash
make clean
make SCHEDULER=MLFQ
make qemu
```

### Exit QEMU
Press `Ctrl-A` then `X`

---

## Testing

### Test System Calls
Inside xv6 shell:
```bash
$ readcount          # Test getreadcount
$ schedulertest      # Test waitx and schedulers
```

### Performance Testing
```bash
# Run CPU-bound processes
$ schedulertest &
$ schedulertest &
$ schedulertest &

# Observe scheduling behavior
# Check process statistics with ps (if implemented)
```

---

## Implementation Details

### File Modifications

**Kernel Files:**
- `kernel/proc.c` - Process scheduler implementations
- `kernel/proc.h` - Process structure modifications
- `kernel/syscall.c` - System call routing
- `kernel/syscall.h` - System call number definitions
- `kernel/sysproc.c` - System call implementations
- `kernel/file.c` - Read count tracking
- `kernel/trap.c` - Timer interrupt handling

**User Files:**
- `user/user.h` - User-space system call declarations
- `user/usys.S` - System call stubs
- `user/readcount.c` - Test program for getreadcount
- `user/schedulertest.c` - Scheduler testing program

### Scheduler Comparison

| Scheduler | Preemptive | Priority | Time Quantum | Starvation Risk |
|-----------|-----------|----------|--------------|-----------------|
| RR        | Yes       | No       | 1 tick       | No              |
| FCFS      | No        | No       | N/A          | No              |
| CFS       | Yes       | No       | Variable     | No              |
| MLFQ      | Yes       | Yes      | 1-16 ticks   | No (aging)      |

### Performance Metrics

**Turnaround Time:** Total time from creation to completion  
**Response Time:** Time from ready to first execution  
**Waiting Time:** Total time spent in ready queue  
**CPU Utilization:** Percentage of time CPU is busy

---

## Project Structure

```
xv6/
├── xv6-source/              # Modified xv6 source
│   ├── kernel/
│   │   ├── proc.c           # Scheduler implementations
│   │   ├── proc.h           # Process structure
│   │   ├── syscall.c        # System call routing
│   │   ├── sysproc.c        # System call handlers
│   │   └── file.c           # Read tracking
│   ├── user/
│   │   ├── readcount.c      # Test program
│   │   └── schedulertest.c  # Scheduler test
│   └── Makefile             # Build with scheduler selection
│
├── kernel/                  # Additional kernel files
│   ├── mlfq_scheduler.c     # MLFQ implementation
│   ├── proc_init.c          # Process initialization
│   └── ...
│
├── IMPLEMENTATION_GUIDE.md  # Detailed modification guide
├── TOOLCHAIN_SETUP.md       # RISC-V toolchain setup
├── report.md                # Performance analysis
└── README.md                # This file
```

---

## Scheduler Details

### FCFS Implementation
```c
// Non-preemptive, runs processes in order
struct proc *p;
struct proc *first = 0;
uint64 min_ctime = 0xffffffffffffffff;

for(p = proc; p < &proc[NPROC]; p++) {
  if(p->state == RUNNABLE) {
    if(p->ctime < min_ctime) {
      min_ctime = p->ctime;
      first = p;
    }
  }
}
if(first) {
  first->state = RUNNING;
  c->proc = first;
  swtch(&c->context, &first->context);
}
```

### CFS Implementation
```c
// Select process with minimum vruntime
struct proc *p;
struct proc *min_vruntime_proc = 0;
uint64 min_vruntime = 0xffffffffffffffff;

for(p = proc; p < &proc[NPROC]; p++) {
  if(p->state == RUNNABLE) {
    if(p->vruntime < min_vruntime) {
      min_vruntime = p->vruntime;
      min_vruntime_proc = p;
    }
  }
}
if(min_vruntime_proc) {
  // Update vruntime after execution
  min_vruntime_proc->vruntime += runtime;
}
```

### MLFQ Implementation
```c
// 5 priority queues with different time quanta
int time_quanta[] = {1, 2, 4, 8, 16};

// Check queues from highest to lowest priority
for(int priority = 0; priority < 5; priority++) {
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p->state == RUNNABLE && p->queue == priority) {
      // Run process with appropriate time quantum
      p->ticks_executed++;
      
      // Move to lower priority if exceeded quantum
      if(p->ticks_executed >= time_quanta[priority]) {
        if(p->queue < 4) p->queue++;
        p->ticks_executed = 0;
      }
    }
  }
}

// Aging: boost priority of starving processes
for(p = proc; p < &proc[NPROC]; p++) {
  if(p->wait_time > AGING_THRESHOLD) {
    if(p->queue > 0) p->queue--;
    p->wait_time = 0;
  }
}
```

---

## Performance Analysis

### Test Methodology
1. Created multiple CPU-bound processes
2. Measured turnaround time, waiting time, response time
3. Compared scheduler behavior under different loads
4. Analyzed fairness and throughput

### Key Findings
- **RR**: Best response time, fair CPU distribution
- **FCFS**: Low overhead, poor for interactive tasks
- **CFS**: Excellent fairness, good for multi-tasking
- **MLFQ**: Adaptive, best for mixed workloads

See [report.md](report.md) for detailed analysis.

---

## Troubleshooting

**Problem:** Build fails with "riscv64-unknown-elf-gcc not found"  
**Solution:** Install RISC-V toolchain (see Prerequisites)

**Problem:** QEMU doesn't start  
**Solution:** Install qemu-system-misc package

**Problem:** Scheduler not switching  
**Solution:** Ensure SCHEDULER macro is defined correctly in Makefile

**Problem:** System calls not working  
**Solution:** Verify all files are modified and kernel is rebuilt

---

## References

- xv6 RISC-V Book: https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf
- Linux CFS: https://www.kernel.org/doc/html/latest/scheduler/sched-design-CFS.html
- MLFQ: Operating Systems: Three Easy Pieces (Chapter 8)

---

## Author

Implementation for OSN Mini-Project 1  
International Institute of Information Technology  

**Note:** This is a teaching/learning project based on MIT's xv6 operating system.
