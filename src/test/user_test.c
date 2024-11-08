/** This is the second test case written for lab 4.
 * Good luck!
 */
#include <test/test.h>
#include <common/rc.h>
#include <common/string.h>
#include <kernel/pt.h>
#include <kernel/mem.h>
#include <kernel/printk.h>
#include <common/sem.h>
#include <kernel/proc.h>
#include <kernel/syscall.h>
#include <driver/memlayout.h>
#include <kernel/sched.h>

extern void trap_return(u64);
extern int start_with_kcontext(Proc *p);
extern u64 proc_entry(void (*entry)(u64), u64 arg);
extern PTEntriesPtr get_pte(struct pgdir *pgdir, u64 va, bool alloc);
extern Semaphore myrepot_done;

static u64 proc_cnt[22] = { 0 }, cpu_cnt[4] = { 0 };

static void vm_test_cp()
{
    printk("vm_test\n");
    static void *p[100000];
    extern RefCount kalloc_page_cnt;
    struct pgdir pg;
    int p0 = kalloc_page_cnt.count;
    init_pgdir(&pg);
    for (u64 i = 0; i < 100000; i++) {
        p[i] = kalloc_page();
        *get_pte(&pg, i << 12, true) = K2P(p[i]) | PTE_USER_DATA;
        *(int *)p[i] = i;
    }
    attach_pgdir(&pg);
    for (u64 i = 0; i < 100000; i++) {
        ASSERT(*(int *)(P2K(PTE_ADDRESS(*get_pte(&pg, i << 12, false)))) ==
               (int)i);
        ASSERT(*(int *)(i << 12) == (int)i);
    }
    free_pgdir(&pg);
    attach_pgdir(&pg);
    for (u64 i = 0; i < 100000; i++)
        kfree_page(p[i]);
    ASSERT(kalloc_page_cnt.count == p0);
    printk("vm_test PASS\n");
}

static void user_test()
{
    printk("user_proc_test\n");
    init_sem(&myrepot_done, 0);
    extern char loop_start[], loop_end[];
    int pids[22];
    for (int i = 0; i < 22; i++) {
        auto p = create_proc();
        for (u64 q = (u64)loop_start; q < (u64)loop_end; q += PAGE_SIZE) {
            *get_pte(&p->pgdir, EXTMEM + q - (u64)loop_start, true) =
                    K2P(q) | PTE_USER_DATA;
        }
        ASSERT(p->pgdir.pt);

        // setup the kernel context
        // you can reference to proc.c: start_proc
        memset(&p->kcontext, 0, sizeof(KernelContext));
        // make room for user context on the stack.
        p->kcontext.sp = (uintptr_t)(p->kstack + PAGE_SIZE);
        p->kcontext.sp -= sizeof(UserContext);
        ASSERT(p->kcontext.sp % 0x10 == 0);
        p->kcontext.lr = (uintptr_t)proc_entry;
        p->kcontext.x1 = 0;
        p->kcontext.x0 = (uint64_t)trap_return;
        // TODO: setup the user context
        // 1. set x0 = i
        // 2. set elr = EXTMEM
        // 3. set spsr = 0
        UserContext *uc = (void *)p->kcontext.sp;
        memset(uc, 0, sizeof(UserContext));
        uc->x0 = i;
        uc->spsr = 0;
        uc->elr = EXTMEM;

        // pids[i] = start_proc(p, trap_return, 0);
        pids[i] = start_with_kcontext(p);
        printk("pid[%d] = %d\n", i, pids[i]);
    }
    ASSERT(wait_sem(&myrepot_done));
    printk("done\n");
    for (int i = 0; i < 22; i++)
        ASSERT(kill(pids[i]) == 0);
    for (int i = 0; i < 22; i++) {
        int code;
        int pid = wait(&code);
        printk("pid %d killed\n", pid);
        ASSERT(code == -1);
    }
    printk("user_proc_test PASS\nRuntime:\n");
    for (int i = 0; i < 4; i++)
        printk("CPU %d: %llu\n", i, cpu_cnt[i]);
    for (int i = 0; i < 22; i++)
        printk("Proc %d: %llu\n", i, proc_cnt[i]);
}

static void rt_entry() {
    vm_test_cp();
    user_test();
    while (1) {
        yield();
    }
}

// for stand-alone testing
void test_init() {
    extern Proc root_proc;
    root_proc.kcontext.x0 = (uint64_t)rt_entry;
}

void run_test() {
}
