/**
 * Current page allocator simply returns nullptr if there's
 * no page available. Since we use seperate page allocator for
 * each cpu, some cpus will run out of page frames faster than 
 * the other. To ensure even overhead, page stealing mechanism 
 * should be used, i.e. when a cpu run out of page frames, it
 * steals one from other cpu.
 * 
 * This test checks implementation of page stealing, we allocate 
 * all pages to cpu 3 and none for others. Then, cpu 0, 1, 2 
 * concurrently "steal" pages from cpu 3, but the page allocator 
 * still have to ensure that pages can be allocated for cpu 3.
 */
#include "test.h"
#include "test_util.h"
#include "sync.h"
#include <common/debug.h>
#include <fdutil/palloc.h>
#include <kernel/shutdown.h>

/** Each cpu allocate this many pages */
#define CPUPG 1024
static void *pages[NCPU][CPUPG];

void test_init()
{
    static int npgs[] = { 0, 0, 0, CPUPG * NCPU };
    palloc_init_limit((int *)npgs);
    sync_init();
}

static void chk_page();

void run_test()
{
    // start test
    TEST_START;
    const int i = cpuid();
    if (i == 0) {
        printk("steal test starting with %d cpu\n", ncpu_all());
    }
    sync(1);

    // allocate phase
    for (int k = 0; k < CPUPG; k++) {
        void *pg = palloc_get();
        if (pg == NULL) {
            PANIC("alloc");
        }
        if (pg_off(pg) != 0) {
            PANIC("alignment");
        }
        pages[i][k] = pg;
    }
    sync(2);

    // validate allocation
    if (i == 2) {
        printk("end alloc phase\n");
        ASSERT(palloc_used() == CPUPG * NCPU);
        chk_page();
        printk("end alloc validation\n");
    }
    sync(3);

    // free phase
    for (int k = 0; k < CPUPG; k++) {
        palloc_free(pages[i][k]);
        pages[i][k] = NULL;
    }
    sync(4);

    // validate free
    if (i == 0) {
        printk("end free phase\n");
        ASSERT(palloc_used() == 0);
        printk("end validate free\n");
    }
    sync(5);

    // ok
    if (i == 0) {
        TEST_END;
    }
}

// check that no page is allocated twice;
// page alignment, nullptr.
static void chk_page()
{
    for (int i = 0; i < NCPU; i++) {
        for (int j = 0; j < CPUPG; j++) {
            for (int k = i; k < NCPU; k++) {
                for (int l = j + 1; l < CPUPG; l++) {
                    if (pages[i][j] == pages[k][l]) {
                        printk("note: i, j, k, l = %d %d %d %d\n", i, j, k, l);
                        PANIC("double allocation detected");
                    }
                }
            }
        }
    }
}
