#pragma once
#ifndef __COMMON_DEBUG_
#define __COMMON_DEBUG_

#include <kernel/printk.h>

// clang-format off
/**
 * What if your kernel crashed?
 * - Sit comfortably;
 * - Check what went wrong;
 * - Check again what went wrong;
 * - Check again and again what went wrong;
 * 
 * Your friends:
 * - ASSERT and STATIC_ASSERT;
 * - hexdump to investigate a bulk of memory;
 * - backtracer;
 * - gdb(-multiarch)
 * 
 * Remember, it is the job, no hard feelings.
 */
// clang-format on

#define BACKTRACE    \
    do {             \
        backtrace(); \
    } while (0)

#ifdef ASSERT
#undef ASSERT
#endif

// assertion
#define ASSERT(cond)                                          \
    do {                                                      \
        if (!(cond)) {                                        \
            printk("Assertion failure: " #cond                \
                   " at [File %s] [Function %s] [Line %d]\n", \
                   __FILE__, __FUNCTION__, __LINE__);         \
            backtrace();                                      \
            for (;;) {                                        \
            }                                                 \
        }                                                     \
    } while (0)

#define STATIC_ASSERT(cond) \
    do {                    \
        switch (0) {        \
        case (0):           \
        case (cond):        \
            break;          \
        }                   \
    } while (0)

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

/** Check that the memory is set to a value. 
 * @param size size of memory block
 * @param val expected value of memory.
 * @return 1 if memory is set to a value.
 */
int Memchk(const unsigned char *start, unsigned long size, int val);

#endif // __COMMON_DEBUG_
