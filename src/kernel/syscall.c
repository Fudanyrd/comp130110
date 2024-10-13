#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <common/sem.h>
#include <test/test.h>
#include <aarch64/intrinsic.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"

void *syscall_table[NR_SYSCALL] = {
    [0 ... NR_SYSCALL - 1] = NULL,
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
        // ???
        context->x0 = (uint64_t)(-1);
        break;
    }
    }
    return;
}

#pragma GCC diagnostic pop