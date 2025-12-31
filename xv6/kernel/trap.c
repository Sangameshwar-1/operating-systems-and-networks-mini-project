// Trap handler updates for scheduler
// ...existing code...

void
usertrap(void)
{
  // ...existing code...
  
  // Update runtime for CFS
  #ifdef SCHEDULER_CFS
  if(p != 0) {
    p->runtime++;
  }
  #endif
  
  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2) {
    #ifdef SCHEDULER_CFS
    // Check if time slice expired for CFS
    int runnable_count = 0;
    for(struct proc *pp = proc; pp < &proc[NPROC]; pp++){
      acquire(&pp->lock);
      if(pp->state == RUNNABLE) runnable_count++;
      release(&pp->lock);
    }
    int target_latency = 48;
    int time_slice = target_latency / (runnable_count > 0 ? runnable_count : 1);
    if(time_slice < 3) time_slice = 3;
    
    if(p->runtime % time_slice == 0)
      yield();
    #else
    yield();
    #endif
  }

  usertrapret();
}

// ...existing code...
