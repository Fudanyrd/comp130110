#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

// thread context
struct context {
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
};

// store the current context in old, restore that of new
extern void swtch(struct context *old, struct context *new);

// kernel cpu
struct cpu {
    struct context context;
    struct proc *running; // current running proc
};

static struct cpu mycpu;

enum proc_stat { UNUSED = 0, RUNNABLE, RUNNING, BLOCKED, ZOMBIE };

struct proc {
    char name[16]; // for debugging
    struct context context; // thread context
    enum proc_stat stat; // status
    unsigned int pid; // id
    void *chan; // channel
    char stk[4096]; // stack
};

// allow 4 threads
#define NPROC 4
static struct proc procs[NPROC];
static unsigned int nextid = 1;

static unsigned int proc_init(const char *name, void *entry)
{
    int found = 0;
    struct proc *proc = NULL;

    assert(name != NULL && entry != NULL);
    for (int i = 0; i < NPROC; i++) {
        if (procs[i].stat == UNUSED) {
            proc = &procs[i];
            proc->pid = nextid++;
            // set up stack and return addr
            proc->context.sp = (uintptr_t)proc->stk + sizeof(proc->stk);
            proc->context.sp -= proc->context.sp % 0x10;
            proc->context.lr = (uintptr_t)entry;
            // set its state to runnable
            proc->stat = RUNNABLE;
            break;
        }
    }

    if (proc == NULL) {
        // cannot allocate
        return -1;
    }
    return proc->pid;
}

// yield the cpu
static void proc_yield()
{
    int off = mycpu.running - (struct proc *)procs;
    procs[off].stat = RUNNABLE;
    // switch to 'kernel'
    swtch(&(procs[off].context), &mycpu.context);
    return;
}

static void proc_exit()
{
    int off = mycpu.running - (struct proc *)procs;
    procs[off].stat = UNUSED;

    // wakeup sleeping threads
    for (int i = 0; i < NPROC; i++) {
        if (procs[i].stat == BLOCKED && procs[i].chan == &procs[i]) {
            // wake the thread up
            procs[i].chan = NULL;
            procs[i].stat = RUNNABLE;
        }
    }
    // switch to 'kernel'
    swtch(&(procs[off].context), &mycpu.context);
}

// wait for proc pid to finish;
// returns -1 if no process has id pid.
static int proc_wait(int pid)
{
    // block execution
    int off = mycpu.running - (struct proc *)procs;
    procs[off].stat = BLOCKED;

    struct proc *p = NULL;
    for (int i = 0; i < NPROC; i++) {
        if (procs[i].pid == pid) {
            p = &procs[i];
        }
    }
    if (p == NULL) {
        // not found, or exit
        return -1;
    }
    procs[off].chan = p;

    // switch to 'kernel'
    swtch(&(procs[off].context), &mycpu.context);
    return 0;
}

static void syswrite(const char *fmt)
{
    // switch to 'kernel'
    proc_yield();

    // perform syscall: write
    puts(fmt);
}

// select one thread to run, if none found,
// return.
static void scheduler()
{
    static int found = 0;
    for (;;) {
        found = 0;

        // look up thread
        for (int i = 0; i < NPROC; i++) {
            if (procs[i].stat == RUNNABLE) {
                found = 1;
                procs[i].stat = RUNNING;
                mycpu.running = &procs[i];
                swtch(&mycpu.context, &procs[i].context);
            }
        }

        if (found == 0) {
            // finish
            return;
        }
    }
}

void foo()
{
    for (int i = 0; i < 10; i++) {
        printf("foo\n");
        proc_yield();
    }
    proc_exit();
}
void bar()
{
    // wait for proc foo to finish!
    proc_wait(2);
    for (int i = 0; i < 10; i++) {
        printf("bar\n");
        proc_yield();
    }
    proc_exit();
}

int main(int argc, char **argv)
{
    memset(&mycpu, 0, sizeof(mycpu));
    memset((char *)procs, 0, sizeof(procs));
    proc_init("foo", (void *)foo);
    proc_init("bar", (void *)bar);
    printf("started foo bar\n");

    scheduler();
    printf("finished foo bar\n");
    return 0;
}
