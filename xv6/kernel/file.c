#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "file.h"

extern uint64 total_bytes_read;

int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;
  // ...existing code...
  r = f->ip->readi(f->ip, 0, addr, f->off, n);
  if(r > 0)
    f->off += r;
  // ...existing code...
  if(r > 0) {
    total_bytes_read += r;
  }
  return r;
}
// ...existing code...
