#include "palloc.h"
#include "stddef.h"
#include <common/debug.h>
#include <common/spinlock.h>
#include <driver/memlayout.h>
#include <driver/base.h>

struct page {
    struct page *nxt;
    char fre[0];
};

/** Page allocator(O(1) for all operations) */
struct pallocator {
    struct page *frepg; /* pointer to free page. */
    SpinLock lock; /* lock */

    /**< Immutable members(don't acquire lock) */
    size_t npage; /* number of pages */
    void *start; /* Start address */
    void *end; /* End address */
} allocators[NCPU];

/** Check for heap buffer overflow */
#define CHECK_PA(pa)                                       \
    do {                                                   \
        if (pa == NULL || pg_off(pa->start) != 0 ||        \
            (pa->end != pa->start + pa->npage * PGSIZE)) { \
            PANIC("Fatal: page allocator corrupted!");     \
        }                                                  \
    } while (0)
// Hint: if CHECK_PA fires, use gdb watch point to find out when
// the page allocator is corrupted.

/** Initialize the page allocator */
void pallocator_init(struct pallocator *pa, void *start, size_t npage);

/** Get a page from allocator */
void *pallocator_get(struct pallocator *pa);

/** Free a page to allocator(must from pallocator_get) */
void pallocator_free(struct pallocator *pa, void *pg);

void palloc_init(void)
{
    // start of heap
    extern char end[];
    ASSERT(end != NULL);

    // first page
    void *first = pg_round_up((void *)end);
    // last page(exclusive)
    void *last = (void *)P2V(PHYSTOP);
    last = pg_round_down(last);
    // alignment
    ASSERT(first < last && pg_off(first) == 0 && pg_off(last) == 0);
    const size_t npgs = (last - first) / PGSIZE;

    // initialize allocators
    size_t npg = npgs / NCPU + npgs % NCPU;
    pallocator_init(&allocators[0], first, npg);
    first += PGSIZE * npg;
    for (int i = 1; i < NCPU; i++) {
        npg = npgs / NCPU;
        pallocator_init(&allocators[i], first, npg);
        // advance.
        first += PGSIZE * npg;
    }

    // check that no page is wasted.
    ASSERT(first == last);
}

void *palloc_get(void)
{
    const int cpu = cpuid();
    // currently just allocate from its own allocator.
    // do not steal from other allocators.
    return pallocator_get(&allocators[cpu]);
}

void palloc_free(void *pg)
{
    // it is NOT safe to use cpuid() to index into
    // the allocator, for one page allocated from
    // a cpu may be freed from another.
    //
    // However, this is true only if we implemented the
    // page stealing mechanism.

    if (pg == NULL) {
        return;
    }

    for (int i = 0; i < NCPU; i++) {
        if (allocators[i].start <= pg && allocators[i].end > pg) {
            pallocator_free(&allocators[i], pg);
            return;
        }
    }
    PANIC("Fatal: cannot find allocator");
}

/**
 * Pallocator implementation
 */
void pallocator_init(struct pallocator *pa, void *start, size_t npage)
{
    ASSERT(pa != NULL && start != NULL && npage != 0);
    pa->start = start;
    pa->end = start + (npage * PGSIZE);
    pa->npage = npage;
    init_spinlock(&pa->lock);

    // initially no free page.
    pa->frepg = NULL;

    // put all pages onto free list.
    void *pg = start;
    for (size_t i = 0; i < npage; i++) {
        pallocator_free(pa, pg);
        pg += PGSIZE;
    }

    CHECK_PA(pa);
}

void *pallocator_get(struct pallocator *pa)
{
    // check that pa is valid
    CHECK_PA(pa);
    ASSERT(pg_off(pa->frepg) == 0);
    acquire_spinlock(&pa->lock);

    // no page available!
    if (pa->frepg == NULL) {
        release_spinlock(&pa->lock);
        return NULL;
    }

    // extract the first free page
    void *ret = pa->frepg;
    pa->frepg = pa->frepg->nxt;

    // return
    release_spinlock(&pa->lock);
    ASSERT(ret >= pa->start && ret < pa->end);
    ASSERT(pg_off(pa->frepg) == 0);
    return ret;
}

void pallocator_free(struct pallocator *pa, void *pg)
{
    // check that pa is valid
    CHECK_PA(pa);

    // validate pg
    if (pg == NULL) {
        // it is valid to free nullptr.
        return;
    }
    ASSERT(pg_off(pg) == 0);
    ASSERT(pg >= pa->start && pg < pa->end);

    // take the lock
    acquire_spinlock(&pa->lock);

    struct page *p = pg;
    p->nxt = pa->frepg;
    pa->frepg = p;

    // release the lock, done.
    release_spinlock(&pa->lock);
}
