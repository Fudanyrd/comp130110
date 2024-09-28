/** Another pstree test.
 * 
 * This time we create a very high process tree, 
 * and the closer one process is to the root, 
 * the later it exits.
 */

#include <kernel/proc.h>
#include <kernel/sched.h>

#define NPR 63

extern Proc root_proc;
static Proc *procs[NPR];

static void rt_entry(u64 unused __attribute__((unused)));
static void pr_entry(u64 unused __attribute__((unused)));
static inline void set_parent(Proc *parent, Proc *child)
{
    ASSERT(child->parent == NULL);
    child->parent = parent;
    list_push_back(&parent->children, &child->ptnode);
}

void test_init()
{
    root_proc.kcontext.x0 = (uint64_t)rt_entry;
    for (int i = 0; i < NPR; i++) {
        procs[i] = create_proc();
        ASSERT(procs[i] != NULL);
        set_parent((i == 0 ? &root_proc : procs[i - 1]), procs[i]);
        start_proc(procs[i], pr_entry, 0);
    }
}

void run_test()
{
    if (cpuid() == 0) {
        printk("start second proc tree test\n");
    }
    yield();
}

static void rt_entry(u64 unused __attribute__((unused)))
{
    int code;
    int pid = wait(&code);
    printk("(%d) excited with %d.\n", pid, code);
    ASSERT(pid > 0);
    yield();
    ASSERT(wait(&code) < 0);
    exit(0);
}

static void pr_entry(u64 unused __attribute__((unused)))
{
    int code;
    int pid = wait(&code);
    printk("(%d) excited with %d.\n", pid, code);
    if (pid < 0) {
        exit(-1);
    }
    yield();
    ASSERT(wait(&code) < 0);
    exit(0);
}
