#include <aarch64/intrinsic.h>
#include <common/string.h>
#include <kernel/printk.h>
#include <test/test.h>
#include <fdutil/malloc.h>
#include "sync.h"

/**
 * Note to TAs: this file is borrowed from previous FDU-OS project.
 */

#define kalloc_page palloc_get
#define kfree_page palloc_free
#define kalloc malloc
#define kfree free

#define NALLOC 10000

static void *p[4][10000];
static short sz[4][10000];

#define SYNC(cnt) sync(cnt);
// #undef SYNC
// #define SYNC(cnt) printk("reach sync %d\n", cnt);

#define FAIL(...)            \
    {                        \
        printk(__VA_ARGS__); \
        while (1)            \
            ;                \
    }

static void alloc2023_test()
{
    int i = cpuid();
    int y = 10000 - i * 500;
    if (cpuid() == 0) {
        printk("starting alloc_test\n");
    }
    SYNC(1)
    for (int j = 0; j < y; j++) {
        p[i][j] = kalloc_page();
        if (!p[i][j] || ((u64)p[i][j] & 4095))
            FAIL("FAIL: alloc_page() = %p\n", p[i][j]);
        memset(p[i][j], i ^ j, PAGE_SIZE);
    }
    for (int j = 0; j < y; j++) {
        u8 m = (i ^ j) & 255;
        for (int k = 0; k < PAGE_SIZE; k++)
            if (((u8 *)p[i][j])[k] != m)
                FAIL("FAIL: page[%d][%d] wrong\n", i, j);
        kfree_page(p[i][j]);
    }
    SYNC(2)
    if (cpuid() == 0) {
        printk("# pages allocated = %u\n", (unsigned)palloc_used());
    }
    /*
    if (alloc_page_cnt.count != r)
        FAIL("FAIL: alloc_page_cnt %d -> %lld\n", r, alloc_page_cnt.count); */
    SYNC(3)
    for (int j = 0; j < NALLOC;) {
        if (j < 1000 || rand() > RAND_MAX / 16 * 7) {
            int z = 0;
            int r = rand() & 255;
            if (r < 127) { // [17,64]
                z = rand() % 48 + 17;
                z = round_up((u64)z, 4ll);
            } else if (r < 181) { // [1,16]
                z = rand() % 16 + 1;
            } else if (r < 235) { // [65,256]
                z = rand() % 192 + 65;
                z = round_up((u64)z, 8ll);
            } else if (r < 255) { // [257,512]
                z = rand() % 256 + 257;
                z = round_up((u64)z, 8ll);
            } else { // [513,2040]
                z = rand() % 1528 + 513;
                z = round_up((u64)z, 8ll);
            }
            sz[i][j] = z;
            p[i][j] = kalloc(z);
            // printk("get %p\n", p[i][j]);
            u64 q = (u64)p[i][j];
            if (p[i][j] == NULL || ((z & 1) == 0 && (q & 1) != 0) ||
                ((z & 3) == 0 && (q & 3) != 0) ||
                ((z & 7) == 0 && (q & 7) != 0))
                FAIL("FAIL: alloc(%d) = %p\n", z, p[i][j]);
            memset(p[i][j], i ^ z, z);
            j++;
        } else {
            int k = rand() % j;
            if (p[i][k] == NULL)
                FAIL("FAIL: block[%d][%d] null\n", i, k);
            int m = (i ^ sz[i][k]) & 255;
            for (int t = 0; t < sz[i][k]; t++)
                if (((u8 *)p[i][k])[t] != m)
                    FAIL("FAIL: block[%d][%d] wrong\n", i, k);
            // printk("free %p\n", p[i][k]);
            kfree(p[i][k]);
            p[i][k] = p[i][--j];
            sz[i][k] = sz[i][j];
        }
    }
    SYNC(4)
    if (cpuid() == 0) {
        i64 z = 0;
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 10000; k++)
                z += sz[j][k];
    }
    if (cpuid() == 0) {
        printk("# pages allocated = %u\n", (unsigned)palloc_used());
    }
    SYNC(5)
    for (int j = 0; j < NALLOC; j++)
        kfree(p[i][j]);
    SYNC(6)
    if (cpuid() == 0) {
        printk("# pages allocated = %u\n", (unsigned)palloc_used());
        printk("alloc_test PASS\n");
    }
}

void test_init(void)
{
    // only initialize sync module
    sync_init();
}

void run_test()
{
    alloc2023_test();
}
