#include "test.h"
#include "test_util.h"
#include "range.h"
#include "sync.h"
#include <common/debug.h>
#include <fdutil/malloc.h>
#include <fdutil/stddef.h>

#define NBLK 8

typedef struct range range_t;
static range_t rgs[NBLK * NCPU];

// malloc smoke test
static void malloc_test(void)
{
    static const int sizes[NCPU] = { 126, 250, 64, 32 };
    TEST_START;
    const int cpu = cpuid();

    // malloc phase
    for (int i = 0; i < NBLK; i++) {
        void *blk = malloc(sizes[cpu]);
        ASSERT(blk != NULL);
        const int idx = i + NBLK * cpu;
        rgs[idx].valid = 1;
        rgs[idx].size = sizes[cpu];
        rgs[idx].start = blk;
    }
    sync(1);

    // free phase
    if (cpu == 0) {
        // check intersection
        for (int i = 0; i < NBLK * NCPU; i++) {
            ASSERT(rg_find(&rgs[0], &rgs[i], NBLK * NCPU) == RG_ERR);
            free(rgs[i].start);
            rgs[i].valid = 0;
        }
    }

    if (cpu == 0) {
        ASSERT(palloc_used() == 0);
        TEST_END;
    }
}

void test_init(void)
{
    // only initialize sync module
    sync_init();
}

void run_test()
{
    malloc_test();
}
