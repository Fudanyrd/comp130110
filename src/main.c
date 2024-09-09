#include <aarch64/intrinsic.h>
#include <driver/uart.h>
#include <kernel/printk.h>
#include <kernel/core.h>
#include <common/string.h>
#include <common/debug.h>
#include <test/test_util.h>
#include <test/test.h>

static volatile bool boot_secondary_cpus = false;

#define HELLO(core)                               \
    do {                                          \
        printk("Hello world! (Core %d)\n", core); \
    } while (0)

#ifdef ASSERT
#undef ASSERT
// assertion
#define ASSERT(cond)                                        \
    do {                                                    \
        if (!(cond)) {                                      \
            printk("Assertion failure: " #cond              \
                   " at [File %s] [Function %s] [Line %d]", \
                   __FILE__, __FUNCTION__, __LINE__);       \
            for (;;) {                                      \
            }                                               \
        }                                                   \
    } while (0)
#endif

void main()
{
    /* edata is .bss start addr */
    extern char edata[];
    /* end is .bss end addr */
    extern char end[];
    ASSERT(end != NULL && edata != NULL);
    ASSERT((void *)end >= (void *)edata);

    if (cpuid() == 0) {
        /* @todo: Clear BSS section.*/
        memset(edata, 0, (usize)(end - edata));
        /* @end todo */

        smp_init();
        uart_init();
        printk_init();
        debug_init();

        /* @todo: Print "Hello, world! (Core 0)" */
        HELLO(0);
        /* @end todo */

        arch_fence();

        // Set a flag indicating that the secondary CPUs can start executing.
        boot_secondary_cpus = true;
        // start running test
        run_test();
    } else {
        while (!boot_secondary_cpus)
            ;
        arch_fence();

        /* @todo: Print "Hello, world! (Core <core id>)" */
        HELLO((int)cpuid());
        /* @end todo */
        // start running test
        run_test();
    }

    set_return_addr(idle_entry);
}
