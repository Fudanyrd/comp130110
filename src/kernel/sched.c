#include <kernel/sched.h>
#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/printk.h>
#include <aarch64/intrinsic.h>
#include <kernel/cpu.h>
#include <fdutil/lst.h>
#include <common/rbtree.h>

/** Scheduler lock */
static SpinLock sched_lock;

/** Scheduler queue */
static struct list queue;

extern bool panic_flag;

extern void swtch(KernelContext *new_ctx, KernelContext *old_ctx);

void init_sched()
{
    // TODO: initialize the scheduler
    // 1. initialize the resources (e.g. locks, semaphores)
    init_spinlock(&sched_lock);
    list_init(&queue);

    // 2. initialize the scheduler info of each CPU
    for (int i = 0; i < NCPU; i++) {
        cpus[i].idle = &idleproc[i];
        cpus[i].proc = NULL;
        // mark as idle thread
        idleproc[i].idle = 1;
        idleproc[i].state = RUNNING;
    }
}

// Returns the currently running process.
// an interesing thing: at first call to sched(),
// cpu->proc is NULL, hence return idle.
Proc *thisproc()
{
    // TODO: return the current process
    struct cpu *cpu = mycpu();
    return cpu->proc != NULL ? cpu->proc : cpu->idle;
}

void init_schinfo(struct schinfo *p)
{
    // TODO: initialize your customized schinfo for every newly-created process
}

void acquire_sched_lock()
{
    // TODO: acquire the sched_lock if need
    acquire_spinlock(&sched_lock);
}

void release_sched_lock()
{
    // TODO: release the sched_lock if need
    release_spinlock(&sched_lock);
}

bool is_zombie(Proc *p)
{
    bool r;
    acquire_sched_lock();
    r = p->state == ZOMBIE;
    release_sched_lock();
    return r;
}

// activate process p.
// in the inside will hold sched_lock.
bool activate_proc(Proc *p)
{
    // TODO:
    // if the proc->state is RUNNING/RUNNABLE, do nothing
    // if the proc->state if SLEEPING/UNUSED, set the process state to RUNNABLE and add it to the sched queue
    // else: panic
    switch (p->state) {
    case (RUNNING):
    case (RUNNABLE): {
        break;
    }
    case (SLEEPING):
    case (UNUSED): {
        p->state = RUNNABLE;
        // push operation is atomic.
        acquire_spinlock(&sched_lock);
        list_push_back(&queue, &p->schq);
        release_spinlock(&sched_lock);
        break;
    }
    default: {
        PANIC("activate zombie proc");
    }
    }
    return true;
}

// update state of current thread.
// Must hold sched_lock.
static void update_this_state(enum procstate new_state)
{
    // TODO: if you use template sched function, you should implement this routinue
    // update the state of current process to new_state, and not [remove it from the sched queue if
    // new_state=SLEEPING/ZOMBIE]

    // set state to new state.
    struct Proc *p = thisproc();
    p->state = new_state;

    switch (new_state) {
    case (RUNNING): {
        PANIC("update to running");
        break;
    }
    case (UNUSED): {
        PANIC("update to unused");
    }
    case (RUNNABLE): {
        // ok
        // if not idle, push back to the queue
        // no need to remove from queue,
        // since it is done in pick_next().
        if (!p->idle) {
            list_push_back(&queue, &p->schq);
        }
        break;
    }
    case (ZOMBIE):
    case (SLEEPING): {
        // remove from the sched queue.
        // lock already held.
        break;
    }
    default:
        break;
    }
}

// Returns next thread to run. Must hold sched_lock.
static Proc *pick_next()
{
    // TODO: if using template sched function, you should implement this routinue
    // choose the next process to run, and return idle if no runnable process
    if (list_empty(&queue)) {
        return mycpu()->idle;
    }
    struct list_elem *e = list_pop_front(&queue);
    return list_entry(e, struct Proc, schq);
}

// update the result of `thisproc` to p.
// This is already implemented for we use
// mycpu()->proc as the result.
static void update_this_proc(Proc *p)
{
    // TODO: you should implement this routinue
    // update thisproc to the choosen process
}

// A simple scheduler.
// You are allowed to replace it with whatever you like.
// call with sched_lock
void sched(enum procstate new_state)
{
    auto this = thisproc();
    ASSERT(this->state == RUNNING);
    update_this_state(new_state);
    auto next = pick_next();
    update_this_proc(next);
    ASSERT(next != NULL);
    ASSERT(next->state == RUNNABLE);
    mycpu()->proc = next;
    next->state = RUNNING;
    if (next != this) {
        swtch(&this->kcontext, &next->kcontext);
    }
    release_sched_lock();
}

u64 proc_entry(void (*entry)(u64), u64 arg)
{
    // at sched() will hold sched_lock,
    // hence need to release it to avoid deadlock.
    // [PITFALL]
    release_sched_lock();
    set_return_addr(entry);
    // return arg will put arg into x0,
    // and the proc can retrieve its parameter.
    return arg;
}
