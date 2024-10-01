/** Trap Test
 * Generate a trap and see the response from trap handler.
 */
#include <aarch64/intrinsic.h>
#include <kernel/cpu.h>
#include <kernel/printk.h>

void test_init()
{
}

void run_test()
{
    if (cpuid() == 0) {
        printk("Trap test\n");
    }
    // enable trap
    set_cpu_on();
    _arch_enable_trap();

    // generate trap!
    *(int *)0x0 = 0x12;
    printk("reach here...\n");
}
