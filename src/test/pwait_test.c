/** Smoke test for wait. Create a parent proc p, and 
 * several child proc foo, bar, baz. p wait for them to finish.
 */
#include <kernel/proc.h>
#include <kernel/sched.h>

static struct Proc *pfoo, *pbar, *ppr;

static void proc_foo(u64 unused __attribute__((unused)))
{
    // now no release_sched_lock is call in
    // proc_entry(), hence deadlock avoided.
    // [NOTE TO TA]: excellent solution, thanks!

    // release_sched_lock();
    for (int i = 0; i < 4; i++) {
        printk("(%d) Hello from foo!\n", thisproc()->pid);
        yield();
    }
    exit(thisproc()->pid + 2);
    PANIC("Reach after exit");
}
static void proc_bar(u64 unused __attribute__((unused)))
{
    // release_sched_lock();
    for (int i = 0; i < 4; i++) {
        printk("(%d) Hello from bar!\n", thisproc()->pid);
        yield();
    }
    exit(thisproc()->pid + 2);
    PANIC("Reach after exit");
}

// parent proc
static void proc_pr(u64 unused __attribute__((unused)))
{
    int code;
    // set parent of foo, bar to parent.
    set_parent_to_this(pfoo);
    set_parent_to_this(pbar);

    for (int i = 0; i < 2; i++) {
        int pid = wait(&code);
        // check exit code
        ASSERT(pid > 0);
        ASSERT(code == pid + 2);
    }
    ASSERT(wait(&code) == -1);
    exit(0);
    PANIC("Reach after exit");
}

void test_init(void)
{
    pfoo = create_proc();
    pbar = create_proc();
    ppr = create_proc();
    ASSERT(pfoo != NULL && pbar != NULL);
    start_proc(pfoo, proc_foo, 0);
    start_proc(pbar, proc_bar, 0);
    start_proc(ppr, proc_pr, 0);
}

void run_test()
{
    // current proc is idle.
    // when idle takes over, it is
    // safe to shutdown.
    if (cpuid() == 0) {
        printk("pwait test\n");
    }
    yield();
    release_sched_lock();
}
