#pragma once

#include <kernel/proc.h>
#include <common/rbtree.h>

#ifndef NCPU
#define NCPU 4
#endif // Add an unfortunate patch. Hmm??@!
#include "proc.h"

struct sched {
    // TODO: customize your sched info
};

struct Proc;
struct cpu {
    bool online;
    struct rb_root_ timer;
    struct sched sched;
    struct Proc *proc; // currently running proc
    struct Proc *idle; // idle proc
};

extern struct cpu cpus[NCPU];
extern struct Proc idleproc[NCPU];

void set_cpu_on();
void set_cpu_off();

/** Returns current cpu. */
static inline struct cpu *mycpu(void)
{
    const int id = cpuid();
    return &cpus[id];
}
