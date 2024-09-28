/** Reparent test: concurrency
 * Create several parent, each with some children. 
 * Then parent exit without waiting for its children.
 * Then root proc will have to resolve all the parent and children.
 * Then these will have to be released.
 * 
 * After root proc finishes, the kernel enters idle proc,
 * which shutdown the machine.
 * 
 * If success, all children exit with code 0. All parent exit with 
 * its index in the parents array.
 */
#include <kernel/proc.h>
#include <kernel/sched.h>

// number of parent
#define NPR 6
// number of children
#define NCHD 8
// number of proc
#define NPROC (NPR + NCHD * NPR)

extern Proc root_proc;
static Proc *parents[NPR];
static Proc *children[NPR][NCHD];

// code executed by root
static void rt_entry();
// code executed by parent
static void pr_entry();
// code executed by children.
static void chd_entry();

void test_init()
{
    root_proc.kcontext.x0 = (uint64_t)rt_entry;

    for (int i = 0; i < NPR; i++) {
        parents[i] = create_proc();
        ASSERT(parents[i] != NULL);
        start_proc(parents[i], pr_entry, 0);
    }
    for (int i = 0; i < NPR; i++) {
        for (int j = 0; j < NCHD; j++) {
            // reparent these children
            children[i][j] = create_proc();
            children[i][j]->parent = parents[i];
            list_push_back(&parents[i]->children, &children[i][j]->ptnode);

            ASSERT(children[i][j] != NULL);
            start_proc(children[i][j], chd_entry, 0);
        }
    }
}

void run_test(void)
{
    if (cpuid() == 0) {
        printk("third proc reparent test\n");
    }
    yield();
}

static void rt_entry()
{
    int code;
    // wake up parent

    // wait for parents to complete.
    for (int i = 0; i < NPROC; i++) {
        // post sema to wakeup a child
        int pid = wait(&code);
        printk("proc %d excited with %d\n", pid, code);
        if (pid < 0) {
            PANIC("lost wakeup detected");
        }
    }
    exit(0);
}

static void pr_entry()
{
    int id = -1;
    for (int i = 0; i < NPR; i++) {
        if (parents[i] == thisproc()) {
            id = i;
            break;
        }
    }
    if (id < 0) {
        PANIC("(parent) not found");
    }
    yield();
    exit(id);
    PANIC("reach after exit");
}

static void chd_entry()
{
    exit(0);
}
