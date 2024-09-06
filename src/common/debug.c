#include "debug.h"
#include <common/defines.h>

void debug_init(void)
{
    // check some assumptions about the ISA.
    STATIC_ASSERT(sizeof(int) == 4);
    STATIC_ASSERT(sizeof(unsigned long) == 8);
    STATIC_ASSERT(sizeof(void *) == 8);

    // it is still highly recommended NOT to make any
    // assumptions about the ISA.

    // if there's other static members, initialize them.
}

/** Returns the next frame pointer. */
static inline void *next_fptr(void *fptr);

/** Returns the current program counter. */
static inline void *fptr2addr(void *fptr);

void backtrace(void)
{
    // the current frame pointer.
    void *fptr;
    // Arm ref manual page 34: x29 is frame pointer
    // asm volatile("ldr %0, [sp]" : "=r"(fptr));
    asm volatile("mov %0, x29" : "=r"(fptr));

    // clang-format off
    /**
     * calling stack:
     * +-------+
     * |  x30  |  
     * +-------+
     * |  x29  |
     * +-------+ <------------o
     * ...                    |
     * +-------+              |
     * |  x30  |              |
     * +-------+              |
     * |  x29  | -------------o
     * +-------+ 
     */
    // clang-format on

    printk("kernel backtrace: ");
    while (fptr != NULL) {
        printk("%p ", fptr2addr(fptr));
        fptr = next_fptr(fptr);
    }
    printk("\n");
}

static inline void *next_fptr(void *fptr)
{
    return *(void **)(fptr);
}

static inline void *fptr2addr(void *fptr)
{
    return *(void **)(fptr + 0x8);
}
