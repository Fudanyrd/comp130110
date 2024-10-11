#include <kernel/sched.h>
#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/printk.h>
#include <aarch64/intrinsic.h>
#include <kernel/cpu.h>
#include <fdutil/lst.h>
#include <common/rbtree.h>

/** Scheduler lock guards the sched queue and the state
 * of each process. Read/write to proc stat must hold
 * the lock. */
static SpinLock sched_lock;

#ifndef RCC
/** Scheduler queue */
static struct list queue;
#else
/** Scheduler queues for all cores */
static struct list queue[NCPU];
#endif

extern bool panic_flag;

extern void swtch(KernelContext *new_ctx, KernelContext *old_ctx);

void init_sched()
{
    // TODO: initialize the scheduler
    // 1. initialize the resources (e.g. locks, semaphores)
    init_spinlock(&sched_lock);
#ifndef RCC
    list_init(&queue);
#else
    for (int i = 0; i < NCPU; i++) {
        list_init(&queue[i]);
    }
#endif

    // 2. initialize the scheduler info of each CPU
    for (int i = 0; i < NCPU; i++) {
        // initialize cpu
        cpus[i].idle = &idleproc[i];
        cpus[i].proc = NULL;
        cpus[i].noff = 0;
        // mark as idle thread
        idleproc[i].idle = 1;
        idleproc[i].state = RUNNING;
        idleproc[i].kstack = NULL;
        list_init(&idleproc[i].children);
        init_sem(&idleproc[i].childexit, 0);
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
    // Note to TA: since the initialization of schinfo relies
    // on the proc's id, it is done in proc.c
}

#ifdef RCC
int update_schinfo(struct schinfo *sch)
{
    // this process is similar to clock algorithm
    ASSERT(sch->coreid < NCPU);
    while (1) {
        if ((sch->bitmap & (1U << sch->coreid)) != 0) {
            // clear this scheduled bit.
            // this means that we notice the proc is once scheduled
            // on the core, so don't run on it again.
            sch->bitmap &= ~(1U << sch->coreid);
        } else {
            // this bit is not set, meaning that the proc
            // has not be scheduled on that core. so return that.
            return sch->coreid;
        }
        // set the position of the frame pointer
        sch->coreid = (sch->coreid + 1) % NCPU;
    }
}
#endif

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
bool is_unused(Proc *p)
{
    bool r;
    acquire_sched_lock();
    r = p->state == UNUSED;
    release_sched_lock();
    return r;
}

bool activate_proc(Proc *p)
{
    // TODO:
    // if the proc->state is RUNNING/RUNNABLE, do nothing
    // if the proc->state if SLEEPING/UNUSED, set the process state to RUNNABLE and add it to the sched queue
    // else: panic
    acquire_spinlock(&sched_lock);
    switch (p->state) {
    case (RUNNING):
    case (RUNNABLE): {
        break;
    }
    case (SLEEPING):
    case (UNUSED): {
        p->state = RUNNABLE;
        // push operation is atomic.
#ifndef RCC
        list_push_back(&queue, &p->schq);
#else
        // I believe before the proc is deactivated,
        // it calls sched() which updates its schinfo.
        // so no update is done here.
        list_push_back(&queue[p->schinfo.coreid], &p->schq);
#endif
        break;
    }
    default: {
        PANIC("activate zombie proc");
    }
    }
    release_spinlock(&sched_lock);
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

#ifdef RCC
    // compute the next core to run the proc.
    if (!p->idle) {
        update_schinfo(&p->schinfo);
    }
#endif

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
#ifndef RCC
            list_push_back(&queue, &p->schq);
#else
            list_push_back(&queue[p->schinfo.coreid], &p->schq);
#endif // RCC
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
#ifndef RCC
    if (list_empty(&queue)) {
        return mycpu()->idle;
    }
    struct list_elem *e = list_pop_front(&queue);
    return list_entry(e, struct Proc, schq);
#else
    const int cid = cpuid();
    if (list_empty(&queue[cid])) {
        // "steal" one proc from other cpu's scheduler queue(?!)
        // if it is not done, some cores at sometime may have
        // no job to run, which is a waste of computation power.
        // It may not also be run with less than 4 cores.

        // it is safe to do so for we already acquired
        // the sched lock.
        for (int i = 1; i < NCPU; i++) {
            const int qid = (cid + i) % NCPU;
            if (!list_empty(&queue[qid])) {
                struct list_elem *e = list_pop_front(&queue[qid]);
                Proc *p = list_entry(e, struct Proc, schq);
                // update its bitmap, this will
                // lower the possibility that it will be scheduled
                // on the same cpu.
                p->schinfo.bitmap |= (1U << cid);
                return p;
            }
        }
        return mycpu()->idle;
    }
    // get a proc from its own scheduler queue
    struct list_elem *e = list_pop_front(&queue[cid]);
    struct Proc *p = list_entry(e, struct Proc, schq);
    // mark the cpu as scheduled.
    p->schinfo.bitmap |= (1U << cid);
    p->schinfo.coreid = (cid + 1) % NCPU;
    return p;
#endif
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
    ASSERT(this->state == RUNNING || this->state == ZOMBIE);
    update_this_state(new_state);
    auto next = pick_next();
    update_this_proc(next);
    ASSERT(next != NULL);
    ASSERT(next->state == RUNNABLE);
    mycpu()->proc = next;
    next->state = RUNNING;
    if (next != this) {
        attach_pgdir(&next->pgdir);
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
