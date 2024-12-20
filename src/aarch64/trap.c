#include <aarch64/trap.h>
#include <aarch64/intrinsic.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <driver/interrupt.h>
#include <kernel/proc.h>
#include <kernel/syscall.h>
#include <kernel/mmap1217.h>

// if trap from kernel: PANIC
// if trap from user space:
// returns 0 if pgfault is handled successfully,
// else should kill user process
static int pgfault_handler(UserContext *ctx, u64 faddr);

/** Returns true if trap is from user space */
static inline int trap_from_user(UserContext *context)
{
    // clang-format off
    // Reference Manual:
    // <https://developer.arm.com/documentation/ddi0601/2024-09/AArch64-Registers/SPSR-EL1--Saved-Program-Status-Register--EL1-?lang=en>
    // To judge whether the trap is from user space, we should 
    // check the M[3:0] bit. 
    
    // * If M[4] is 0: from aarch64
    // M[3:0] = 0b0000 means el0. 
    // M[3:0] = 0b0100 means EL1 with SP_EL0 (EL1t).
    // M[3:0] = 0b0101 means EL1 with SP_EL1 (EL1h).
    // * If M[4] is 1: from aarch32
    // M[3:0] = 0b0000 means user.
    // clang-format on
    return (context->spsr & 0b1111) == 0;
}

/** Print the calling stack of an saved context */
static void trap_bt(UserContext *ctx)
{
    if (trap_from_user(ctx)) {
        // this does not work for user context,
        // since user stack may already be destroyed,
        // and may cause kernel to trigger page fault.
        printk("Trap from user space");
        return;
    }
    // print the address that the intr occurred
    printk("%p ", (void *)ctx->elr);

    // frame pointer
    void *fptr = (void *)ctx->fp;
    void *pc; // program counter
    while (fptr != NULL) {
        pc = *(void **)(fptr + 0x8);
        if (pc != NULL) {
            pc = pc - 0x4;
        }
        printk("%p ", pc);
        fptr = *(void **)fptr;
    }
    printk("\n");
}

void trap_global_handler(UserContext *context)
{
    thisproc()->ucontext = context;

    u64 esr = arch_get_esr();
    u64 ec = esr >> ESR_EC_SHIFT;
    u64 iss = esr & ESR_ISS_MASK;
    u64 ir = esr & ESR_IR_MASK;

    (void)iss;

    arch_reset_esr();

    switch (ec) {
    case ESR_EC_UNKNOWN: {
        if (ir)
            PANIC();
        else
            interrupt_global_handler();
    } break;
    case ESR_EC_SVC64: {
        ASSERT(trap_from_user(context));
        syscall_entry(context);
    } break;
    case ESR_EC_IABORT_EL0:
    case ESR_EC_IABORT_EL1:
    case ESR_EC_DABORT_EL0:
    case ESR_EC_DABORT_EL1: {
        u64 faddr = arch_get_far();
        if (pgfault_handler(context, faddr) < 0) {
            printk("User program %d segfault! Killed\n", thisproc()->pid);
            thisproc()->killed = 1;
        }
    } break;
    default: {
        printk("Unknwon exception %llu\n", ec);
        PANIC();
    }
    }

    // TODO: stop killed process while returning to user space
    if (thisproc()->killed && trap_from_user(context)) {
        // use syscall exit
        context->x0 = -1;
        context->x8 = 11;
        syscall_entry(context);
    }
}

NO_RETURN void trap_error_handler(u64 type)
{
    printk("Unknown trap type %llu\n", type);
    PANIC();
}

static int pgfault_handler(UserContext *ctx, u64 faddr)
{
    struct pgdir *pd = &thisproc()->pgdir;
    faddr = round_down(faddr, PAGE_SIZE);
    if (trap_from_user(ctx)) {
        struct section *sec = section_search(pd, faddr);
        if (sec == NULL) {
            // fail
            return -1;
        }

        return section_install(pd, sec, faddr);
    }

    else {
        printk("FATAL: Page fault\n");
        printk("fault addr: %p\n", (void *)faddr);
        trap_bt(ctx);
        PANIC();
    }
}
