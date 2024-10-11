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
    int noff; // depth of push_off()
    int intena; // interrupt enabled
};

extern struct cpu cpus[NCPU];
extern struct Proc idleproc[NCPU];

struct timer {
    bool triggered;
    int elapse;
    u64 _key;
    struct rb_node_ _node;
    void (*handler)(struct timer *);
    u64 data;
};

void init_clock_handler();

void set_cpu_on();
void set_cpu_off();
void push_off();
void pop_off();

/** Returns current cpu. */
static inline struct cpu *mycpu(void)
{
    const int id = cpuid();
    return &cpus[id];
}

void set_cpu_timer(struct timer *timer);
void cancel_cpu_timer(struct timer *timer);
