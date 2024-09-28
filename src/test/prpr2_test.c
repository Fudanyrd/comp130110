/** Reparent test: not long, but tricky!
 * 
 * The root proc is grandparent, which spawns another proc as parent. 
 * Different from prpr-test, this time we LET CHILDREN EXIT FIRST.
 * 
 * This will check lost(accidental) wakeup and data race in 
 * reparenting impl.
 */

#include <kernel/proc.h>
#include <kernel/sched.h>
#include <common/sem.h>

#define NCHD 47

static Semaphore sema;
static Proc *pr;
static Proc *children[NCHD];
extern Proc root_proc;

// code executed by root proc
static void rt_entry()
{
    for (int i = 0; i < NCHD; i++) {
        yield();
    }

    int code;
    // wake up parent
    post_sem(&sema);

    // wait for
    for (int i = 0; i < NCHD + 1; i++) {
        // post sema to wakeup a child
        int pid = wait(&code);
        printk("proc %d excited with %d\n", pid, code);
        if (pid < 0) {
            PANIC("lost wakeup detected");
        }
    }
    ASSERT(sema.val == 0);
    ASSERT(wait(&code) == -1);
    exit(0);
}

// code executed by parent proc
static void pr_entry()
{
    for (int i = 0; i < NCHD; i++) {
        set_parent_to_this(children[i]);
    }
    wait_sem(&sema);
    exit(0);
}

// code executed by child proc
static void chd_entry()
{
    // exit immediately, becoming a zombie.
    exit(thisproc()->pid + 123);
}

void test_init()
{
    root_proc.kcontext.x0 = (uint64_t)rt_entry;
    init_sem(&sema, 0);

    // create procs
    pr = create_proc();
    start_proc(pr, pr_entry, 0);
    ASSERT(pr != NULL);
    ASSERT(pr->parent == &root_proc);
    for (int i = 0; i < NCHD; i++) {
        children[i] = create_proc();
        ASSERT(children[i] != NULL);
        start_proc(children[i], chd_entry, 0);
        ASSERT(children[i]->parent == &root_proc);
    }
    ASSERT(root_proc.childexit.val == 0);
}

void run_test()
{
    // current proc is idle.
    // when idle takes over, it is
    // safe to shutdown.
    if (cpuid() == 0) {
        printk("proc reparent test\n");
    }
    yield();
}
