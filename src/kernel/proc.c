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
    release_sched_lock();
    while (1) {
        yield();
        release_sched_lock();
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
    // 1. return -1 if no children
    if (list_empty(&p->children)) {
        return -1;
    }
    // 2. wait for childexit
    wait_sem(&p->childexit);
    // 3. if any child exits, clean it up and return its pid and exitcode
    int ret = -1;
    ASSERT(!list_empty(&p->children));
    struct list_elem *e = list_begin(&p->children);
    acquire_spinlock(&pstree_lock);
    for (; e != list_end(&p->children); e = list_next(e)) {
        struct Proc *chd = list_entry(e, struct Proc, ptnode);
        ASSERT(chd->parent == p);
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
            break;
        }
    }
    release_spinlock(&pstree_lock);

    // NOTE: be careful of concurrency
    return ret;
}

NO_RETURN void exit(int code)
{
    // TODO:
    struct Proc *p = myproc;
    // 1. set the exitcode
    p->exitcode = code;
    // 2. clean up the resources
    // shold not call kfree_page, for later
    // sched() will store the context!
    // That will be use-after-free.
    // [PITFALL]

    // 3. transfer children to the root_proc, and notify the root_proc if there is zombie
    acquire_spinlock(&pstree_lock);
    struct list_elem *e;
    while (!list_empty(&p->children)) {
        e = list_pop_front(&p->children);
        list_push_back(&root_proc.children, e);
    }
    release_spinlock(&pstree_lock);

    // 3.1 wakeup its parent
    // This is not described in the comment[PITFALL]
    struct Proc *parent = p->parent;
    if (p != parent) {
        post_sem(&parent->childexit);
    }

    // 4. sched(ZOMBIE)
    acquire_sched_lock();
    sched(ZOMBIE);
    // will not reach
    // NOTE: be careful of concurrency

    PANIC(); // prevent the warning of 'no_return function returns'
}
