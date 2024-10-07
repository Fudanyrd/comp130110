/** This tests the implementation of RCC configuration.
 * 
 * We create several(but not many) threads, ideally given 
 * enough time, they will be scheduled to multiple cores
 * to run.
 * 
 * See kernel/config.h for more info.
 */

#define NLOOP 8
#define NPROC 8

#include <kernel/sched.h>

extern Proc root_proc;
Proc *pr[NPROC];

static void pc_entry()
{
    for (int i = 0; i < NLOOP; i++) {
        printk("(%d) now running on %d.\n", thisproc()->pid, (int)cpuid());
        // yield the cpu so that it can be run on somewhere else.
        yield();
    }
    exit(0);
}

static void rt_entry()
{
    int pid;
    int code;
    for (int i = 0; i < NPROC; i++) {
        pid = (wait(&code));
        ASSERT(pid >= 0);
        printk("(%d) excited with %d.\n", pid, code);
    }
    exit(0);
}

void test_init()
{
    root_proc.kcontext.x0 = (uint64_t)rt_entry;

    for (int i = 0; i < NPROC; i++) {
        pr[i] = create_proc();
        ASSERT(pr[i] != NULL);
        start_proc(pr[i], pc_entry, 0);
    }
}

void run_test()
{
    yield();
}
