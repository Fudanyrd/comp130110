#pragma once
#ifndef __COMMON_DEBUG_
#define __COMMON_DEBUG_

#include <kernel/printk.h>

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

#endif // __COMMON_DEBUG_
