// ...existing code...
#ifdef SCHEDULER_FCFS
// FCFS Scheduler
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
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
// CFS Scheduler
static uint64
calculate_weight(int nice)
{
  // Approximation: weight = 1024 / (1.25 ^ nice)
  // For simplicity, use a lookup table or approximate calculation
  if(nice == 0) return 1024;
  if(nice < 0) {
    // Higher priority: weight increases
    uint64 weight = 1024;
    for(int i = 0; i < -nice; i++)
      weight = (weight * 5) / 4; // multiply by 1.25
    return weight;
  } else {
    // Lower priority: weight decreases
    uint64 weight = 1024;
    for(int i = 0; i < nice; i++)
      weight = (weight * 4) / 5; // divide by 1.25
    if(weight < 15) weight = 15;
    return weight;
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
    
    // Find process with minimum vruntime
    struct proc *min_proc = 0;
    uint64 min_vruntime = -1;
    int runnable_count = 0;
    
    for(p = proc; p < &proc[NPROC]; p++){
      acquire(&p->lock);
      if(p->state == RUNNABLE){
        runnable_count++;
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
      // Log scheduling decision
      printf("[Scheduler Tick]\n");
      for(p = proc; p < &proc[NPROC]; p++){
        acquire(&p->lock);
        if(p->state == RUNNABLE){
          printf("PID: %d | vRuntime: %d\n", p->pid, p->vruntime);
        }
        release(&p->lock);
      }
      printf("--> Scheduling PID %d (lowest vRuntime)\n", min_proc->pid);
      
      // Calculate time slice
      int target_latency = 48;
      int time_slice = target_latency / (runnable_count > 0 ? runnable_count : 1);
      if(time_slice < 3) time_slice = 3;
      
      min_proc->state = RUNNING;
      c->proc = min_proc;
      
      uint64 start_time = min_proc->runtime;
      swtch(&c->context, &min_proc->context);
      
      // Update vruntime after process runs
      uint64 elapsed = min_proc->runtime - start_time;
      min_proc->vruntime += (elapsed * 1024) / min_proc->weight;
      
      c->proc = 0;
      release(&min_proc->lock);
    }
  }
}
#else
// Default Round Robin Scheduler
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
// ...existing code...
