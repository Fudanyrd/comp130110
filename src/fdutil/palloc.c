#include "palloc.h"
#include "stddef.h"
#include <common/debug.h>
#include <common/spinlock.h>
#include <common/string.h>
#include <driver/memlayout.h>
#include <driver/base.h>

/** Organize pages in singly-linked list. */
struct page {
    struct page *nxt;
    char fre[0];
};

// use u8 to do ref count,
// support at most 255 forks, which is large enough
typedef u8 refcnt_t;

#define MAX_REF_CNT ((u8)0xff)

/** Page allocator(O(1) for all operations) */
struct pallocator {
    struct page *frepg; /* pointer to free page. */
    size_t nalloc; /* Number of allocated pages(debug, test) */
    SpinLock lock; /* lock */

    /**< Immutable members(don't acquire lock) */
    size_t npage; /* number of pages, NOT include refcnt */
    void *start; /* Start address */
    void *end; /* End address */
    refcnt_t *refcnts; /* Reference count */
};

// physical page manager.
static struct pallocator allocator;

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

static INLINE refcnt_t *pg2refcnt(void *pg)
{
    ASSERT(pg_off(pg) == 0);
    ASSERT(pg >= allocator.start && pg < allocator.end);
    return allocator.refcnts + (pg - allocator.start) / PAGE_SIZE;
}

/** Initialize the page allocator, Will hold lock */
static void pallocator_init(struct pallocator *pa, void *start, size_t npage);

/** Get a page from allocator. Will hold lock */
static void *pallocator_get(struct pallocator *pa);

/** Free a page to allocator(must from pallocator_get) */
static void pallocator_free(struct pallocator *pa, void *pg);

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

    // leave pages for refcnt.
    // nrf = round_up(npgs, 4096)
    const size_t nrf = (npgs + PAGE_SIZE - 1) / PAGE_SIZE;
    ASSERT(nrf < npgs);
    // init refcount area.
    memset(first, 0, PAGE_SIZE * nrf);

    // update first.
    allocator.refcnts = (u8 *)first;
    first += nrf * PAGE_SIZE;

    // init pallocator.
    pallocator_init(&allocator, first, npgs - nrf);
}

void *palloc_get(void)
{
    void *ret = pallocator_get(&allocator);
    return ret;
}

void palloc_free(void *pg)
{
    return pallocator_free(&allocator, pg);
}

void *palloc_share(void *pg)
{
    ASSERT(pg >= allocator.start && pg < allocator.end);
    ASSERT(pg_off(pg) == 0);

    // fast path: the page count does not overflow.
    acquire_spinlock(&allocator.lock);
    refcnt_t *rc = pg2refcnt(pg);
    ASSERT(*rc > (u8)0);
    if (*rc != (u8)0xff) {
        *rc = *rc + 1;
        // just give the page back.
        release_spinlock(&allocator.lock);
        return pg;
    }
    release_spinlock(&allocator.lock);

    // slow path: should allocate a new page
    // with the same context
    void *ret = pallocator_get(&allocator);
    if (ret != NULL) {
        memcpy(ret, pg, PAGE_SIZE);
    }
    return ret;
}

static INLINE void push_page(struct pallocator *pa, void *pg)
{
    struct page *p = pg;
    p->nxt = pa->frepg;
    pa->frepg = p;
}

/**
 * Pallocator implementation
 */
static void pallocator_init(struct pallocator *pa, void *start, size_t npage)
{
    ASSERT(pa != NULL && start != NULL && npage != 0);
    pa->start = start;
    pa->end = start + (npage * PGSIZE);
    pa->npage = npage;
    pa->nalloc = 0;
    init_spinlock(&pa->lock);

    // initially no free page.
    pa->frepg = NULL;

    // put all pages onto free list.
    void *pg = start;
    for (size_t i = 0; i < npage; i++) {
        push_page(&allocator, pg);
        pg += PGSIZE;
    }

    CHECK_PA(pa);
}

static void *pallocator_get(struct pallocator *pa)
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
    pa->nalloc++;

    // update ref count
    refcnt_t *rc = pg2refcnt(ret);
    ASSERT(*rc == (u8)0);
    *rc = *rc + 1;

    // return
    release_spinlock(&pa->lock);
    ASSERT(ret >= pa->start && ret < pa->end);
    ASSERT(pg_off(pa->frepg) == 0);
    return ret;
}

static void pallocator_free(struct pallocator *pa, void *pg)
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

    // deal with refcount
    acquire_spinlock(&pa->lock);
    refcnt_t *rc = pg2refcnt(pg);
    ASSERT(*rc > (u8)0x0);
    *rc = *rc - 1;
    if (*rc > 0) {
        // it is used by someone else, do NOT free
        release_spinlock(&pa->lock);
        return;
    }
    release_spinlock(&pa->lock);

    // fill with junks, to detect use-after-free.
    memset(pg, 0xcc, PGSIZE);

    // take the lock
    acquire_spinlock(&pa->lock);

    struct page *p = pg;
    p->nxt = pa->frepg;
    pa->frepg = p;
    ASSERT(pa->nalloc > 0);
    pa->nalloc--;

    // release the lock, done.
    release_spinlock(&pa->lock);
}
