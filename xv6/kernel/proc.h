#ifndef XV6_PROC_H
#define XV6_PROC_H

// Per-process state
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID

  // wait_lock must be held when using this:
  struct proc *parent;         // Parent process

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  
  uint64 ctime;                // Creation time for FCFS
  
  // CFS fields
  int nice;                    // Nice value (-20 to 19)
  uint64 vruntime;             // Virtual runtime
  uint64 weight;               // Process weight
  uint64 runtime;              // Actual runtime in ticks
  
  // MLFQ fields (bonus)
  int queue;                   // Current queue level (0-3)
  int time_slice;              // Remaining time slice
  uint64 queue_enter_time;     // When entered current queue
};

#endif
