#pragma once

#include <kernel/printk.h>

#define TEST_START int succ = 1
#define TEST_END \
    do { \
        if (succ) { \
            printk("TEST PASSED\n"); \
        } else { \
            printk("TEST FAILED\n") \
        } \
    } while(0)

// test cases
typedef void testfn(void);

// test debug utilities
void debug_test(void);