#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "file.h"

// Global counter for total bytes read
uint64 total_bytes_read = 0;

// ...existing code...

uint64 sys_getreadcount(void) {
    return total_bytes_read;
}

// ...existing code...
