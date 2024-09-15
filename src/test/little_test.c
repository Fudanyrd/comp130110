#include "test.h"
#include "test_util.h"
#include <fdutil/malloc.h>
#include <fdutil/stddef.h>
#include <common/rc.h>
#include <common/string.h>
#include <common/debug.h>
#include <kernel/mem.h>
#include <kernel/printk.h>

static void *p[NCPU][16];
static int cnt[NCPU];

/**
 * Test the behavior of malloc in extreme condition:
 * few pages are avaialble(in this test, 1 page).
 * 
 * This is where concurrency issues like deadlock,
 * data races are likely to happen.
 */

static RefCount x;
#define SYNC(i)             \
    arch_dsb_sy();          \
    increment_rc(&x);       \
    while (x.count < 4 * i) \
        ;                   \
    arch_dsb_sy();

/** Single page allocator */
struct sallocator {
    void *pg;
    SpinLock lock;
};

static struct sallocator sa;

static void *salloc()
{
    acquire_spinlock(&sa.lock);
    void *ret = sa.pg;
    sa.pg = NULL;
    release_spinlock(&sa.lock);
    return ret;
}

static void sfree(void *pg)
{
    acquire_spinlock(&sa.lock);
    if (pg != NULL) {
        sa.pg = pg;
    }
    release_spinlock(&sa.lock);
}

#define SZ 256

void run_test(void)
{
    const int i = cpuid();
    SYNC(1);

    // alloc phase
    for (;;) {
        void *blk = malloc(SZ);
        if (blk == NULL) {
            break;
        }
        int j = cnt[i];
        p[i][j] = blk;
        memset(blk, i ^ j, SZ);
        cnt[i]++;
    }
    SYNC(2);

    // print statistics
    if (i == 0) {
        // here I did not explicitly check,
        // but ideally blocks is allocated
        // evenly across cpus.
        for (int k = 0; k < NCPU; k++) {
            printk("cpu %d got %d blocks.\n", k, cnt[k]);
        }
    }
    SYNC(3);

    // free phase
    for (int k = 0; k < cnt[i]; k++) {
        void *blk = p[i][k];
        ASSERT(Memchk(blk, SZ, (i ^ k) & 0xff));
        free(blk);
        p[i][k] = NULL;
        free(NULL);
    }
    SYNC(4);

    // end
    if (i == 0) {
        // check that pages is freed to sallocator
        void *pg = salloc();
        ASSERT(pg != NULL && pg_off(pg) == 0);
        sfree(pg);
        printk("TEST PASSED\n");
    }
    SYNC(5);
}

void test_init()
{
    init_spinlock(&sa.lock);
    /* Initliaze with single page */
    sa.pg = palloc_get();

    /* Change malloc's backend page allocator */
    struct palloc_intf intf;
    intf.get = salloc;
    intf.free = sfree;
    intf.getmult = NULL;
    intf.freemult = NULL;
    malloc_mod_intf(intf);

    for (int i = 0; i < NCPU; i++) {
        cnt[i] = 0;
    }
}
