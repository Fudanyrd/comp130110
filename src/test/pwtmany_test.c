/** Stress test: wait for many children to finish execution.
 * 
 * Concurret calls to exit(), wait() and set_parent_to_this() 
 * yield bug.
 */

#include <kernel/proc.h>
#include <kernel/sched.h>
#define NCHLD 24

// children
static struct Proc *children[NCHLD];
// parent
static struct Proc *pr;

static void proc_pr(u64 nchd);
static void proc_chd(u64 code __attribute__((unused)));

void test_init(void)
{
    pr = create_proc();
    ASSERT(pr != NULL);
    for (int i = 0; i < NCHLD; i++) {
        children[i] = create_proc();
        ASSERT(children[i] != NULL);
    }
    start_proc(pr, proc_pr, (u64)NCHLD);
    for (int i = 0; i < NCHLD; i++) {
        start_proc(children[i], proc_chd, 0);
    }
}

// parent proc
static void proc_pr(u64 nchd)
{
    int code;
    // check param passing
    ASSERT(nchd == NCHLD);
    for (int i = 0; i < NCHLD; i++) {
        set_parent_to_this(children[i]);
    }
    // set parent of foo, bar to parent.

    for (int i = 0; i < NCHLD; i++) {
        int pid = wait(&code);
        // check exit code
        ASSERT(pid > 0);
        ASSERT(code == pid + 2);
    }
    ASSERT(wait(&code) == -1);
    exit(0);
    PANIC("Reach after exit");
}

static void proc_chd(u64 code __attribute__((unused)))
{
    for (int i = 0; i < 2; i++) {
        printk("(%d) Hello from proc!\n", thisproc()->pid);
    }
    exit(thisproc()->pid + 2);
}

void run_test()
{
    // current proc is idle.
    // when idle takes over, it is
    // safe to shutdown.
    if (cpuid() == 0) {
        printk("wait many test\n");
    }
    yield();
    release_sched_lock();
}
