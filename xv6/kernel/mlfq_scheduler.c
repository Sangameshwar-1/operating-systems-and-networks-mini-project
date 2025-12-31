// MLFQ Scheduler Implementation (Bonus)
// Add this to kernel/proc.c

#ifdef SCHEDULER_MLFQ

#define QUEUE_COUNT 4
#define BOOST_INTERVAL 48

static int queue_time_slices[QUEUE_COUNT] = {1, 4, 8, 16};

void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    intr_on();
    
    // Priority boost every 48 ticks to prevent starvation
    static uint64 last_boost = 0;
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
      
      // Update time slice and queue
      selected->time_slice -= elapsed;
      if(selected->time_slice <= 0){
        // Move to lower priority queue
        if(selected->queue < QUEUE_COUNT - 1){
          selected->queue++;
        }
        selected->time_slice = queue_time_slices[selected->queue];
      }
      
      c->proc = 0;
      release(&selected->lock);
    }
  }
}
#endif
