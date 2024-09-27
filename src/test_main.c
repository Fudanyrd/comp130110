#include <aarch64/intrinsic.h>
#include <driver/uart.h>
#include <driver/timer.h>
#include <driver/interrupt.h>
#include <driver/gicv3.h>
#include <driver/timer.h>
#include <kernel/proc.h>
#include <kernel/sched.h>
#include <kernel/printk.h>
#include <kernel/shutdown.h>
#include <kernel/core.h>
#include <common/string.h>
#include <common/debug.h>
#include <fdutil/malloc.h>
#include <fdutil/palloc.h>
#include <test/test_util.h>
#include <test/test.h>

/**
 * A main.c that is specialized for building and running tests.
 * 
 * Difference from main.c: it runs tests and calls shutdown at 
 * the end.
 */

static volatile bool boot_secondary_cpus = false;

/**
 * Decide when secondary cpu calls shut_record().
 */
static volatile int shutinit = 0;

#define HELLO(core)                               \
    do {                                          \
        printk("Hello world! (Core %d)\n", core); \
    } while (0)

void main()
{
    /* edata is .bss start addr */
    extern char edata[];
    /* end is .bss end addr */
    extern char end[];

    if (cpuid() == 0) {
        /* @todo: Clear BSS section.*/
        ASSERT(end != NULL && edata != NULL);
        ASSERT((void *)end >= (void *)edata);
        memset(edata, 0, (usize)(end - edata));
        /* @end todo */

        smp_init();
        uart_init();
        printk_init();
        debug_init();

        gicv3_init();
        gicv3_init_percpu();
        timer_init(1000);
        timer_init_percpu();
        init_sched();
        init_kproc();
        smp_init();

        // boot shutdown module
        shut_init();
        shutinit = 1;
        __sync_synchronize();
        shut_record();
        palloc_init();
        malloc_init();
        test_init();

        arch_fence();

        // Set a flag indicating that the secondary CPUs can start executing.
        boot_secondary_cpus = true;
        __sync_synchronize();
        // start running test
        run_test();
    } else {
        while (shutinit == 0) {
        }
        shut_record();
        while (!boot_secondary_cpus)
            ;
        arch_fence();
        timer_init_percpu();
        gicv3_init_percpu();

        // start running test
        run_test();
    }

    // shutdown the cpu, this will call
    shutdown();
    set_return_addr(idle_entry);
}
