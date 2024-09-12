#include "test.h"
#include "test_util.h"
#include <aarch64/intrinsic.h>
#include <common/debug.h>
#include <fdutil/palloc.h>
#include <fdutil/stddef.h>

#define NALLOC 16
static void *pages[NALLOC * NCPU];

/** Smoke test for palloc */
static void palloc_test(void)
{
    // easy, this test is not hard to pass.
    // just allocate NALLOC pages, check intersection,
    // and free them.
    // This is a concurrent test, however.

    TEST_START;
    const int cpu = cpuid();

    for (int i = 0; i < NALLOC; i++) {
        void *pg = palloc_get();
        ASSERT(pg != NULL && pg_off(pg) == 0);

        // check that it does not intersect with previous.
        for (int k = 0; k < i; k++) {
            ASSERT(pages[cpu * NALLOC + k] != pg);
        }
        pages[cpu * NALLOC + i] = pg;
    }

    // free all pages.
    for (int i = 0; i < NALLOC; i++) {
        palloc_free(pages[cpu * NALLOC + i]);
    }
    if (cpu == 0) {
        TEST_END;
    }
}

void run_test(void)
{
    palloc_test();
}
