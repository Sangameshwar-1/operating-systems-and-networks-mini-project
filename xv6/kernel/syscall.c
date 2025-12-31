#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// ...existing code...
extern uint64 sys_getreadcount(void);
// ...existing code...
static uint64 (*syscalls[])(void) = {
// ...existing code...
  [SYS_getreadcount]    sys_getreadcount,
// ...existing code...
};
// ...existing code...
