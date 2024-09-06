#pragma once
#ifndef __COMMON_DEBUG_
#define __COMMON_DEBUG_

#include <kernel/printk.h>

#define BACKTRACE do { backtrace(); } while(0)

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

#define STATIC_ASSERT(cond)                                 \
    do { switch(0) { case (0): case (cond): break; }     \
    } while(0)

/**-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                          DEBUG UTILITIES 
 -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+*/

/** Initialize the debug module */
void debug_init(void);

/** Print a block of memory 
 * @param size size of the block
 * @param off offset of dump
 */
void hexdump(unsigned char *start, unsigned long size, unsigned long off);

/** Print the calling stack. */
void backtrace(void);

#endif // __COMMON_DEBUG_
