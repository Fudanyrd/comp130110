#include <aarch64/intrinsic.h>
#include <driver/uart.h>
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
        // boot shutdown module
        shut_init();
        shutinit = 1;
        shut_record();
        palloc_init();
        malloc_init();
        test_init();

        arch_fence();

        // Set a flag indicating that the secondary CPUs can start executing.
        boot_secondary_cpus = true;
        // start running test
        run_test();
    } else {
        while (shutinit == 0) {
        }
        shut_record();
        while (!boot_secondary_cpus)
            ;
        arch_fence();

        // start running test
        run_test();
    }

    // shutdown the cpu, this will call
    shutdown();
    set_return_addr(idle_entry);
}
