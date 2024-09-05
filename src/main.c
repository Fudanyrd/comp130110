#include <aarch64/intrinsic.h>
#include <driver/uart.h>
#include <kernel/printk.h>
#include <kernel/core.h>
#include <common/string.h>
#include <common/debug.h>

static volatile bool boot_secondary_cpus = false;

#define HELLO(core) do { \
		printk("Hello world! (Core %d)\n", core); \
	} while(0)

#ifdef ASSERT
#undef ASSERT
// assertion
#define ASSERT(cond) \
    do { \
      if (!(cond)) { \
        printk("Assertion failure: " #cond  \
            " at [File %s] [Function %s] [Line %d]" \
            , __FILE__, __FUNCTION__, __LINE__); \
        for (;;) {} \
      }\
    } while(0)
#endif

void main()
{
<<<<<<< HEAD
    if (cpuid() == 0) {
        /* @todo: Clear BSS section.*/
=======
	/* edata is .bss start addr */
	extern char edata[];
	/* end is .bss end addr */
	extern char end[];
	ASSERT (end != NULL && edata != NULL);
	ASSERT (end >= edata);

	if (cpuid() == 0) {
		/* @todo: Clear BSS section.*/
		memset(edata, 0, (usize)(end - edata));
		/* @end todo */
>>>>>>> bc9c530cde6b65659e709c541a7f003f7271747e

        smp_init();
        uart_init();
        printk_init();

<<<<<<< HEAD
        /* @todo: Print "Hello, world! (Core 0)" */
=======
		/* @todo: Print "Hello, world! (Core 0)" */
		HELLO(0);
		/* @end todo */
>>>>>>> bc9c530cde6b65659e709c541a7f003f7271747e

        arch_fence();

        // Set a flag indicating that the secondary CPUs can start executing.
        boot_secondary_cpus = true;
    } else {
        while (!boot_secondary_cpus)
            ;
        arch_fence();

<<<<<<< HEAD
        /* @todo: Print "Hello, world! (Core <core id>)" */
    }
=======
		/* @todo: Print "Hello, world! (Core <core id>)" */
		HELLO((int)cpuid());
		/* @end todo */
	}
>>>>>>> bc9c530cde6b65659e709c541a7f003f7271747e

    set_return_addr(idle_entry);
}
