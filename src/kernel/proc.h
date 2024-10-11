#pragma once

#include <common/defines.h>
#include <common/list.h>
#include <common/sem.h>
#include <common/rbtree.h>
#include <kernel/config.h>

#include <fdutil/stdint.h>
#include <fdutil/stddef.h>
#include <fdutil/lst.h>
#include <kernel/pt.h>

enum procstate { UNUSED, RUNNABLE, RUNNING, SLEEPING, ZOMBIE };

typedef struct UserContext {
    // TODO: customize your trap frame
    uint64_t x0; /*   0 */
    uint64_t x1; /*   8 */
    uint64_t x2; /*  16 */
    uint64_t x3; /*  24 */
    uint64_t x4; /*  32 */
    uint64_t x5; /*  40 */
    uint64_t x6; /*  48 */
    uint64_t x7; /*  56 */
    uint64_t x8; /*  64 */
    uint64_t x9; /*  72 */
    uint64_t x10; /* 80 */
    uint64_t x11; /* 88 */
    uint64_t x12; /* 96 */
    uint64_t x13; /* 104 */
    uint64_t x14; /* 112 */
    uint64_t x15; /* 120 */
    uint64_t x16; /* 128 */
    uint64_t x17; /* 136 */
    uint64_t x18; /* 144 */
    uint64_t x19; /* 152 */
    uint64_t x20; /* 160 */
    uint64_t x21; /* 168 */
    uint64_t x22; /* 176 */
    uint64_t x23; /* 184 */
    uint64_t x24; /* 192 */
    uint64_t x25; /* 200 */
    uint64_t x26; /* 208 */
    uint64_t x27; /* 216 */
    uint64_t x28; /* 224 */
    uintptr_t lr; /* 232 */
    uintptr_t fp; /* 240 */
    uintptr_t sp; /* 248 */
    uintptr_t spsr; /* 256 */
    uintptr_t elr; /* 264 */
    // 272 = 0x110 bytes

} UserContext;

typedef struct KernelContext {
    // TODO: customize your context
    // all callee-saved registers
    uint64_t x8; /*   0 */
    uint64_t x16; /*   8 */
    uint64_t x17; /*  16 */
    uint64_t x18; /*  24 */
    uint64_t x19; /*  32 */
    uint64_t x20; /*  40 */
    uint64_t x21; /*  48 */
    uint64_t x22; /*  56 */
    uint64_t x23; /*  64 */
    uint64_t x24; /*  72 */
    uint64_t x25; /*  80 */
    uint64_t x26; /*  88 */
    uint64_t x27; /*  96 */
    uintptr_t fp; /* 104 */
    uintptr_t lr; /* 112 */
    uintptr_t sp; /* 120 */
    uint64_t x0; /* 128 */
    uint64_t x1; /* 136 */
} KernelContext;

// embeded data for procs
#ifndef RCC
struct schinfo {
    // TODO: customize your sched info
};
#else
/** Scheduler information */
struct schinfo {
    uint32_t coreid; /** cpu it is currently running */
    uint32_t bitmap; /** record scheduled cpu id */
};

/** Update scheduler information, which is done in sched().
 * @return the next cpu to run.
 */
int update_schinfo(struct schinfo *sch);
#endif

typedef struct Proc {
    // must held lock when accessing these
    int killed;
    int idle;
    int pid;
    int exitcode;
    struct Proc *parent;
    struct list children;
    struct list_elem ptnode; // put on parent's children list.
    struct list_elem schq; // put on scheduler queue
    void *chan;

    enum procstate state;
    Semaphore childexit;
    struct schinfo schinfo;
    struct pgdir pgdir;
    void *kstack;
    KernelContext kcontext;
    UserContext *ucontext;
} Proc;

void init_kproc();
void init_kproc_test();
void init_proc(Proc *);
Proc *create_proc();
void set_parent_to_this(Proc *proc);
int start_proc(Proc *, void (*entry)(u64), u64 arg);
NO_RETURN void exit(int code);
int wait(int *exitcode);
int kill(int pid);