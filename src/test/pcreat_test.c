/**
 * Simple smoke test that creates very few processes, 
 * each print a message and yield.  After hello 
 * several times, exit.
 * 
 * This test will pass if output seems like:
 * pcreat test
 * (1) Hello from foo!
 * (2) Hello from bar!
 * (1) Hello from foo!
 * (2) Hello from bar!
 * (1) Hello from foo!
 * (2) Hello from bar!
 * (1) Hello from foo!
 * (2) Hello from bar!
 * [shutdown]
 * 
 * The output is identical to that above if using round-robin
 * scheduler.
 */
#include <kernel/proc.h>
#include <kernel/sched.h>

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
    exit(0);
}
static void proc_bar(u64 unused __attribute__((unused)))
{
    // release_sched_lock();
    for (int i = 0; i < 4; i++) {
        printk("(%d) Hello from bar!\n", thisproc()->pid);
        yield();
    }
    exit(0);
}
static struct Proc *pfoo, *pbar;

void test_init(void)
{
    pfoo = create_proc();
    pbar = create_proc();
    ASSERT(pfoo != NULL && pbar != NULL);
    start_proc(pfoo, proc_foo, 0);
    start_proc(pbar, proc_bar, 0);
}

void run_test()
{
    // current proc is idle.
    // when idle takes over, it is
    // safe to shutdown.
    if (cpuid() == 0) {
        printk("pcreat test\n");
    }
    yield();
    release_sched_lock();
}
