#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/sched.h>
#include <aarch64/mmu.h>
#include <common/list.h>
#include <common/string.h>
#include <kernel/printk.h>
#include <fdutil/palloc.h>
#include <kernel/shutdown.h>

#define myproc thisproc()

/** Turn on/off logging */
#define Log(fmt, ...) printk(fmt, ##__VA_ARGS__)
#undef Log
#define Log(fmt, ...)

static void sleep(void *chan, SpinLock *lock) __attribute__((unused));
static void wakeup(struct Proc *p, void *chan) __attribute__((unused));
static void sleep(void *chan, SpinLock *lock)
{
    acquire_sched_lock();
    myproc->chan = chan;
    release_spinlock(lock);
    sched(SLEEPING);
    // reacquire lock
    acquire_spinlock(lock);
}
static void wakeup(struct Proc *p, void *chan)
{
    if (p->state == SLEEPING && p->chan == chan)
        activate_proc(p);
}

static int nextid = 1;
static SpinLock pid_lock;
static int allocpid()
{
    acquire_spinlock(&pid_lock);
    int ret = nextid++;
    if (nextid < 0) {
        // overflow?!@
        nextid = 1;
    }
    release_spinlock(&pid_lock);
    return ret;
}

/** Lock to process tree */
static SpinLock pstree_lock;

Proc root_proc;

void kernel_entry();
extern void proc_entry(void (*entry)(u64), u64 arg);

// init_kproc initializes the kernel process
// NOTE: should call after kinit
void init_kproc()
{
    // TODO:
    // 1. init global resources (e.g. locks, semaphores)
    init_spinlock(&pstree_lock);
    init_spinlock(&pid_lock);
    nextid = 1;
    // 2. init the root_proc (finished)

    init_proc(&root_proc);
    root_proc.parent = &root_proc;
    root_proc.pid = 0;
    nextid = 1;
    start_proc(&root_proc, kernel_entry, 123456);
}

// when running test, we hope that
// root proc will wait for all its child to finish,
// and call shutdown(), which finishes the test.
static void root_entry(u64 unused __attribute__((unused)))
{
    int code;
    while (1) {
        yield();
        if (wait(&code) < 0) {
            // no more child!
            shutdown();
        }
    }
}

// initialize for running test.
// will not activate root proc.
void init_kproc_test()
{
    init_spinlock(&pstree_lock);
    init_spinlock(&pid_lock);
    nextid = 1;

    init_proc(&root_proc);
    root_proc.parent = &root_proc;
    root_proc.pid = 0;
    nextid = 1;
    start_proc(&root_proc, root_entry, 123456);
}

void init_proc(Proc *p)
{
    // TODO:
    // setup the Proc with kstack and pid allocated
    // NOTE: be careful of concurrency
    p->killed = 0;
    p->idle = 0;
    p->pid = allocpid();
    p->exitcode = 0;
    p->parent = NULL;
    p->chan = NULL;
    list_init(&p->children);
    init_sem(&p->childexit, 0);
    // kstack is allocated in init_proc(),
    // released in exit().
    p->kstack = kalloc_page();
    p->ucontext = NULL;
}

Proc *create_proc()
{
    Proc *p = kalloc(sizeof(Proc));
    init_proc(p);
    return p;
}

void set_parent_to_this(Proc *proc)
{
    // TODO: set the parent of proc to thisproc
    // NOTE: maybe you need to lock the process tree
    // NOTE: it's ensured that the old proc->parent = NULL
    acquire_spinlock(&pstree_lock);
    // [PITFALL]: remove from old parent's children list
    if (proc->parent != NULL) {
        list_remove(&proc->ptnode);
    }
    proc->parent = myproc;
    list_push_back(&myproc->children, &proc->ptnode);
    release_spinlock(&pstree_lock);
}

int start_proc(Proc *p, void (*entry)(u64), u64 arg)
{
    // TODO:
    // 1. set the parent to root_proc if NULL
    if (p->parent == NULL) {
        acquire_spinlock(&pstree_lock);
        p->parent = &root_proc;
        list_push_back(&root_proc.children, &p->ptnode);
        release_spinlock(&pstree_lock);
    }
    // 2. setup the kcontext to make the proc start with proc_entry(entry, arg)
    memset((void *)&p->kcontext, 0, sizeof(p->kcontext));
    ASSERT(pg_off(p->kstack) == 0);
    ASSERT(p->kstack != NULL);

    // setup stack, return addr, param to proc_entry.
    // proc_entry does crutial synchronization updates,
    // see its comments. [PITFALL]
    p->kcontext.sp = (uintptr_t)(p->kstack + PAGE_SIZE);
    p->kcontext.lr = (uintptr_t)proc_entry;
    p->kcontext.x1 = arg;
    p->kcontext.x0 = (uint64_t)entry;
    // 3. activate the proc and return its pid
    p->state = UNUSED;
    activate_proc(p);
    // NOTE: be careful of concurrency
    return p->pid;
}

int wait(int *exitcode)
{
    // TODO:
    ASSERT(exitcode != NULL);
    struct Proc *p = myproc;
    ASSERT(p->state == RUNNING);
    // 1. return -1 if no children
    // [PITFALL] if one proc is writeing pstree,
    // list_empty would get incorrect result.
    acquire_spinlock(&pstree_lock);
    if (list_empty(&p->children)) {
        Log("no children\n");
        release_spinlock(&pstree_lock);
        return -1;
    }
    // [PITFALL]: must release pstree_lock,
    // if its child call exit(), then it will try to
    // acquire pstree_lock, which results in deadlock.
    release_spinlock(&pstree_lock);

    // 2. wait for childexit
    wait_sem(&p->childexit);
    // reacquire lock
    acquire_spinlock(&pstree_lock);
    // 3. if any child exits, clean it up and return its pid and exitcode
    int ret = -1;
    ASSERT(!list_empty(&p->children));
    struct list_elem *e = list_begin(&p->children);
    for (; e != list_end(&p->children); e = list_next(e)) {
        struct Proc *chd = list_entry(e, struct Proc, ptnode);
        // validate child proc
        ASSERT(chd->parent == p);
        ASSERT(chd->pid >= 0);
        Log("(%d): state %d\n", chd->pid, (int)chd->state);
        acquire_sched_lock();
        if (chd->state == ZOMBIE) {
            // remove the child from children list
            list_remove(&chd->ptnode);

            // set miscellaneous item
            *exitcode = chd->exitcode;
            ret = chd->pid;

            // free the proc struct
            // kfree_page is done here, reason
            // is described in exit, "2. clean up the resources"
            kfree_page(chd->kstack);
            kfree(chd);
            release_sched_lock();
            break;
        }
        release_sched_lock();
    }
    release_spinlock(&pstree_lock);
    if (ret < 0) {
        // not found, possibly accedential wakeup.
        PANIC("no zombie found");
    }

    // NOTE: be careful of concurrency
    return ret;
}

NO_RETURN void exit(int code)
{
    // TODO:
    struct Proc *p = myproc;
    // 1. set the exitcode
    // [PITFALL] lock must be held, cause other proc
    // may be reading p->state or p->exitcode.
    // see test case prpr2.
    // Want to ensure that proc woke up in wait can
    // see at least one zombie proc.
    acquire_spinlock(&pstree_lock);
    // p->state = ZOMBIE;
    p->exitcode = code;
    // 2. clean up the resources
    // shold not call kfree_page, for later
    // sched() will store the context!
    // That will be use-after-free.
    // [PITFALL]

    // 3. transfer children to the root_proc, and notify the root_proc if there is zombie
    int rootcnt = 0; // num post_sem
    struct list_elem *e;
    while (!list_empty(&p->children)) {
        e = list_pop_front(&p->children);
        list_push_back(&root_proc.children, e);
        struct Proc *chd = list_entry(e, struct Proc, ptnode);
        ASSERT(chd->state != UNUSED);
        acquire_sched_lock();
        chd->parent = &root_proc;
        // if there is zombei proc, wakeup root proc.
        if (chd->state == ZOMBIE) {
            ++rootcnt;
        }
        release_sched_lock();
    }

    // 3.1 wakeup its parent
    // This is not described in the comment[PITFALL]
    struct Proc *parent = p->parent;
    if (p != parent) {
        post_sem(&parent->childexit);
    }

    // notify the root process of added children.
    for (int i = 0; i < rootcnt; i++) {
        if (p != &root_proc)
            post_sem(&root_proc.childexit);
    }

    // 4. sched(ZOMBIE)
    acquire_sched_lock();
    // we have notified its parent and/or root proc that one of the child
    // is ready. but BEFORE we set its state to zombie, the parent/root
    // proc should NOT read its state. this is done by preempting the
    // sched lock and then release the pstree_lock.

    // release pstree lock allows parent/root to scan its children,
    // but they will have to acquire the sched lock, at which time
    // they can see the zombie state of thisproc().
    release_spinlock(&pstree_lock);
    sched(ZOMBIE);
    // will not reach
    // NOTE: be careful of concurrency

    PANIC(); // prevent the warning of 'no_return function returns'
}
