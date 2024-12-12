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
void syscall_open(UserContext *ctx);
void syscall_close(UserContext *ctx);
void syscall_readdir(UserContext *ctx);
void syscall_read(UserContext *ctx);
void syscall_chdir(UserContext *ctx);
void syscall_mkdir(UserContext *ctx);
void syscall_write(UserContext *ctx);
void syscall_unlink(UserContext *ctx);
void syscall_fork(UserContext *ctx);

/** Page table helper methods. */

void *syscall_table[NR_SYSCALL] = {
    [0]  = NULL,
    [1] = (void *)syscall_print,
    [2] = (void *)syscall_open,
    [3] = (void *)syscall_close,
    [4] = (void *)syscall_readdir,
    [5] = (void *)syscall_read,
    [6] = (void *)syscall_chdir,
    [7] = (void *)syscall_mkdir,
    [8] = (void *)syscall_write,
    [9] = (void *)syscall_unlink,
    [10] = (void *)syscall_fork,
    [11 ... NR_SYSCALL - 1] = NULL,
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

isize copyin(struct pgdir *pd, void *ka, u64 va, u64 size)
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

isize copyout(struct pgdir *pd, void *ka, u64 va, u64 size)
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
        memcpy(src, ka, ncp);

        // advance
        ka += ncp;
        va += ncp;
        size -= ncp;
    }

    return size;
}

extern isize copyinstr(struct pgdir *pd, void *ka, u64 va)
{
    ASSERT(IS_KERNEL_ADDR(ka));
    usize count = PAGE_SIZE;

    while (count > 0) {
        usize ncp = PAGE_SIZE - va % PAGE_SIZE;
        ncp = ncp > count ? count : ncp;

        PTEntry *entr = get_pte(pd, va, false);
        if (entr == NULL) {
            // error 
            return -1;
        }

        void *src = (void *)(*entr & (~0xfffUL)); 
        src = (void *)P2K(src);
        src += va % PAGE_SIZE;

        for (usize i = 0; i < ncp; i++) {
            *(char *)ka = *(char *)src; 
            // advance
            if (*(char *)ka == 0) {
                break;
            }
            count++;
            src++;
            ka++;
        }

        if (*(char *)ka == 0) {
            // done.
            break;
        }

        // advance(ka, count advanced in inner loop)
        va += ncp;
    }

    // count = 0 marks failure.
    return count == 0 ? -1 : 0;
}

void syscall_open(UserContext *ctx)
{
    // note: 
    // int sys_open(const char *path, int flags);
    char *buf = kalloc_page();
    copyinstr(&thisproc()->pgdir, buf, ctx->x0);
    int ret = sys_open(buf, (int)ctx->x1);
    kfree_page(buf);
    ctx->x0 = (u64)ret;
}

void syscall_close(UserContext *ctx)
{
    // note:
    // int sys_close(int fd);
    ctx->x0 = sys_close((int)ctx->x0);
}

void syscall_readdir(UserContext *ctx)
{
    // note:
    // int sys_readdir(int fd, char *buf);
    DirEntry *dir = kalloc(sizeof(DirEntry));
    int ret = sys_readdir((int)ctx->x0, (char *)dir);
    u64 uva = ctx->x1;
    if (ret >= 0 && 
        copyout(&thisproc()->pgdir, dir, uva, sizeof(DirEntry)) != 0) {
        ret = -1;
    }
    kfree(dir);
    ctx->x0 = (int)ret;
}

void syscall_read(UserContext *ctx)
{
    // note:
    // int sys_read(int fd, char *buf, usize count);
    char *buf = kalloc_page();
    if (buf == NULL) {
        ctx->x0 = -1;
        return;
    }

    isize ret = 0;
    isize tmp;
    // user may require more than a page's data.
    // must satisfy.
    while (ctx->x2 > PAGE_SIZE) {
        tmp = sys_read(ctx->x0, buf, PAGE_SIZE);
        if (tmp < 0) {
            goto rd_bad;
        }
        if (copyout(&thisproc()->pgdir, buf, ctx->x1, tmp) != 0) {
            goto rd_bad;
        }

        // advance
        ctx->x1 += PAGE_SIZE;
        ctx->x2 -= PAGE_SIZE;
        ret += tmp;
    }
    tmp = sys_read((int)ctx->x0, buf, ctx->x2);
    if (tmp < 0) {
        goto rd_bad;
    }
    if (copyout(&thisproc()->pgdir, buf, ctx->x1, tmp) != 0) {
        goto rd_bad;
    }
    ret += tmp;
    kfree_page(buf);
    ctx->x0 = ret;
    return;

rd_bad:
    kfree_page(buf);
    ctx->x0 = -1;
    return;
}

void syscall_chdir(UserContext *ctx)
{
    // note:
    // int sys_chdir(const char *path);
    char *buf = kalloc_page();
    struct pgdir *pd = &thisproc()->pgdir;
    int ret;
    if (buf == NULL) {
        ctx->x0 = -1;
        return;
    }
    if (copyinstr(pd, buf, ctx->x0) != 0) {
        kfree_page(buf);
        ctx->x0 = -1;
        return;
    }
    ret = sys_chdir(buf);
    kfree_page(buf);
    ctx->x0 = ret;
}

void syscall_mkdir(UserContext *ctx)
{
    // hint:
    // int sys_mkdir(const char *path);
    char *buf = kalloc_page();
    if (buf == NULL) {
        // fail
        ctx->x0 = -1;
        return;
    }

    struct pgdir *pd = &thisproc()->pgdir;
    if (copyinstr(pd, buf, ctx->x0) != 0) {
        ctx->x0 = -1;
        kfree_page(buf);
        return; 
    }
    ctx->x0 = sys_mkdir(buf);
    kfree_page(buf);
}

void syscall_write(UserContext *ctx)
{
    // note:
    // isize sys_write(int fd, char *buf, usize count);
    char *buf = kalloc_page();
    if (buf == NULL) {
        // fail
        ctx->x0 = -1;
        return;
    }
    struct pgdir *pd = &thisproc()->pgdir;

    isize ret = 0;
    isize tmp;

    // can only copyin 4096 bytes a time.
    while (ctx->x2 > PAGE_SIZE) {
        if (copyin(pd, buf, ctx->x1, PAGE_SIZE) != 0) {
            goto wrt_bad;
        }

        // do the write.
        tmp = sys_write(ctx->x0, buf, PAGE_SIZE);
        if (tmp < 0) {
            goto wrt_bad;
        }

        // advance
        ctx->x2 -= PAGE_SIZE;
        ctx->x1 += PAGE_SIZE;
        ret += tmp;
    }
    // copyin the rest.
    if (copyin(pd, buf, ctx->x1, ctx->x2) != 0) {
        goto wrt_bad;
    }
    tmp = sys_write(ctx->x0, buf, ctx->x2);
    if (tmp < 0) {
        goto wrt_bad;
    }
    ret += tmp;

    kfree_page(buf);
    ctx->x0 = (u64)ret;
    return;

    // something unexpected happened.
wrt_bad:
    kfree_page(buf);
    ctx->x0 = -1;
    return;
}

void syscall_unlink(UserContext *ctx)
{
    // note:
    // int sys_unlink(const char *target);

    char *buf = kalloc_page();
    if (buf == NULL) {
        ctx->x0 = -1;
        return;
    }

    struct pgdir *pd = &thisproc()->pgdir;
    if (copyinstr(pd, buf, ctx->x0) != 0) {
        // fail
        kfree_page(buf);
        ctx->x0 = -1;
        return;
    }

    ctx->x0 = sys_unlink(buf);
    kfree_page(buf);
    return;
}

extern int fork();

void syscall_fork(UserContext *ctx)
{
    ctx->x0 = fork();
    return;
}

#pragma GCC diagnostic pop