# Makefile for xv6 with SCHEDULER macro support
# ...existing code...

ifndef SCHEDULER
SCHEDULER=RR
endif

CFLAGS += -DSCHEDULER_$(SCHEDULER)

# ...existing code...

# Usage:
#   make SCHEDULER=FCFS
#   make SCHEDULER=CFS
#   make SCHEDULER=MLFQ
