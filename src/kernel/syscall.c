#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <kernel/pt.h>
#include <common/sem.h>
#include <common/string.h>
#include <test/test.h>
#include <aarch64/intrinsic.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"

/** Syscall executor type */
typedef void (*syscall_fn)(UserContext *);

void syscall_print(UserContext *ctx);

/** Page table helper methods. */

/**
 * @param ka kernel virtual address as dst
 * @param va user virtual address, provided in user context
 * @param size num bytes to copy into kernel
 * @return 0 on success.
 */
static isize copyin(struct pgdir *pd, void *ka, u64 va, u64 size);

void *syscall_table[NR_SYSCALL] = {
    [0]  = NULL,
    [1] = (void *)syscall_print,
    [2 ... NR_SYSCALL - 1] = NULL,
    [SYS_myreport] = (void *)syscall_myreport,
};

void syscall_entry(UserContext *context)
{
    // TODO
    // Invoke syscall_table[id] with args and set the return value.
    // id is stored in x8. args are stored in x0-x5. return value is stored in x0.
    // be sure to check the range of id. if id >= NR_SYSCALL, panic.
    const uint64_t id = context->x8;
    if (id >= NR_SYSCALL || syscall_table[id] == NULL) {
        PANIC("no such syscall");
    }
    switch (id) {
    case (SYS_myreport): {
        context->x0 = syscall_myreport(context->x0);
        break;
    }
    default: {
        // function that executes the syscall
        syscall_fn fn = syscall_table[id]; 
        fn(context);
        break;
    }
    }
    return;
}

// prototype:
// isize print(const char *addr, u64 len);
void syscall_print(UserContext *ctx)
{
    if (ctx->x1 >= PAGE_SIZE) {
        // since this is a debug method,
        // and we cannot alloc buffer, fail
        ctx->x0 = -1;
        return;
    }

    Proc *p = thisproc();
    struct pgdir *pd = &p->pgdir;
    if (ctx->x1 < 2048) {
        char *buf = kalloc(ctx->x1 + 1);
        copyin(pd, buf, ctx->x0, ctx->x1);
        buf[ctx->x1] = 0;
        printk("[USER] %s\n", buf);
        kfree(buf);
    } else {
        char *buf = kalloc_page();
        copyin(pd, buf, ctx->x0, ctx->x1);
        buf[ctx->x1] = 0;
        printk("[USER] %s\n", buf);
        kfree_page(buf);
    }
}

static isize copyin(struct pgdir *pd, void *ka, u64 va, u64 size)
{
    if (size == 0) {
        return 0;
    }

    while (size > 0) {
        // n copy in this round.
        usize ncp = PAGE_SIZE - va % PAGE_SIZE;
        // ncp = min(ncp, size)
        ncp = ncp > size ? size : ncp;

        PTEntry *entr = get_pte(pd, va, false);
        if (entr == NULL) {
            // error
            return -1;
        }

        void *src = (void *)(*entr & (~0xfffUL)); 
        src = (void *)P2K(src);
        src += va % PAGE_SIZE;
        memcpy(ka, src, ncp);

        // advance
        ka += ncp;
        va += ncp;
        size -= ncp;
    }

    return size;
}

#pragma GCC diagnostic pop