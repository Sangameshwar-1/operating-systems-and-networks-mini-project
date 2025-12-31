// Process initialization code
// ...existing code...

static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  // Initialize scheduler fields
  p->ctime = ticks;  // Creation time for FCFS
  p->nice = 0;       // Default nice value for CFS
  p->vruntime = 0;   // Initialize virtual runtime
  p->weight = calculate_weight(0);  // Calculate initial weight
  p->runtime = 0;    // Initialize runtime
  
  // MLFQ fields (bonus)
  p->queue = 0;      // Start in highest priority queue
  p->time_slice = 1; // Initial time slice
  p->queue_enter_time = ticks;

  return p;
}

// ...existing code...
