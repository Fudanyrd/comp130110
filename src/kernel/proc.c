#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/sched.h>
#include <aarch64/mmu.h>
#include <common/list.h>
#include <common/string.h>
#include <kernel/printk.h>
#include <fdutil/palloc.h>

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
void proc_entry();

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
    init_proc(&root_proc);
    root_proc.pid = 0;
    nextid = 1;
    start_proc(&root_proc, kernel_entry, 123456);
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
    p->kcontext.sp = (uintptr_t)(p->kstack + PAGE_SIZE);
    p->kcontext.lr = (uintptr_t)entry;
    p->kcontext.x0 = arg;
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
    int ret = 0;
    ASSERT(!list_empty(&p->children));
    struct list_elem *e = list_begin(&p->children);
    for (; e != list_end(&p->children); e = list_next(e)) {
        struct Proc *chd = list_entry(e, struct Proc, ptnode);
        ASSERT(chd->parent == p);
        if (chd->state == ZOMBIE) {
            // remove the child from children list
            acquire_spinlock(&pstree_lock);
            list_remove(&chd->ptnode);

            // set miscellaneous item
            *exitcode = chd->exitcode;
            ret = chd->pid;
            release_spinlock(&pstree_lock);

            // free the proc struct
            kfree(chd);
            break;
        }
    }
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
    kfree_page(p->kstack);
    // 3. transfer children to the root_proc, and notify the root_proc if there is zombie
    acquire_spinlock(&pstree_lock);
    struct list_elem *e;
    while (!list_empty(&p->children)) {
        e = list_pop_front(&p->children);
        list_push_back(&root_proc.children, e);
    }
    release_spinlock(&pstree_lock);

    // 4. sched(ZOMBIE)
    sched(ZOMBIE);
    // will not reach
    // NOTE: be careful of concurrency

    PANIC(); // prevent the warning of 'no_return function returns'
}
