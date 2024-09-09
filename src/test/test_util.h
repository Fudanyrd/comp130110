#pragma once

#include <kernel/printk.h>
#include <aarch64/intrinsic.h>

#define TEST_START int succ = 1
#define TEST_END                     \
    do {                             \
        if (succ) {                  \
            printk("TEST PASSED\n"); \
        } else {                     \
            printk("TEST FAILED\n"); \
        }                            \
    } while (0)

#define LOG(fmt, ...)                                                  \
    printk("%s:%d In %s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, \
           ##__VA_ARGS__)

// test cases
typedef void testfn(void);

// test debug utilities
void debug_test(void);

// test bitmap impl
void bitmap_test(void);
