#include "malloc.h"
#include "stdint.h"
#include "lst.h"
#include <common/spinlock.h>
#include <common/string.h>
#include <common/rc.h>

/** For testing */
extern RefCount kalloc_page_cnt;

/** A descriptor */
struct desc {
    struct list flst; // list of free blocks
    SpinLock lock;

    /** Immutable members */
    uint32_t bsz; // size of blocks
    uint32_t bpa; // blocks per arena
};

static void add_blocks(struct desc *d);

/** Number of descriptors */
#define NDESC 8

/** An arena */
struct arena {
    struct desc *desc; // descriptor
    uint32_t nfr; // number of free block
    uint32_t magic; // magic number
};

/** Returns the arena managing the blk. */
static inline struct arena *block2arena(void *blk);

/** Free the space of an arena */
static void arena_free(struct arena *a);

static struct desc descs[NDESC];

/** Arena magic number */
#define ARENA_MAGIC 0x9a548eed

void malloc_init(void)
{
    // initialize 8 descriptors, each handles memory
    // block of size:
    // 16, 32, 64, 128, 256
    // 512, 1024, 2048

    uint32_t sz = 16U;
    for (int i = 0; i < NDESC; i++) {
        descs[i].bsz = sz;
        descs[i].bpa = (PGSIZE - sizeof(struct arena)) / sz;
        init_spinlock(&descs[i].lock);
        list_init(&descs[i].flst);
        ASSERT(list_empty(&descs[i].flst));
        sz *= 2;
    }
}

void *malloc(size_t nb)
{
    if (nb == 0) {
        // zero-sized buffer
        return NULL;
    }
    // round nb up to power of 2.
    struct desc *d = NULL;
    void *ret;

    for (int i = 0; i < NDESC; i++) {
        if (nb <= descs[i].bsz) {
            d = &descs[i];
            break;
        }
    }

    // no fitting descriptor.
    if (d == NULL) {
        return NULL;
    }

    acquire_spinlock(&d->lock);
    if (list_empty(&d->flst)) {
        // fetch another page, and put its blocks
        // onto the free list of the descriptor.
        // must hold the lock when doing so.
        add_blocks(d);
    }
    ASSERT(!list_empty(&d->flst));

    ret = (void *)list_pop_front(&d->flst);
    struct arena *ar = block2arena(ret);
    ASSERT(ar->nfr > 0);
    ar->nfr--;

    release_spinlock(&d->lock);
    return ret;
}

/** Currently free does nothing. */
void free(void *pt __attribute__((unused)))
{
    if (pt == NULL) {
        return;
    }

    // get arena and descriptor
    struct arena *ar = block2arena(pt);
    struct desc *d = ar->desc;
    ASSERT(d != NULL);
    acquire_spinlock(&d->lock);
    ASSERT(ar->nfr < d->bpa);
    ar->nfr++;

    // make it easier to trigger use-after-free
    memset(pt, 0xcc, d->bsz);
    list_push_back(&d->flst, (struct list_elem *)pt);
    if (ar->nfr == d->bpa) {
        // free the arena.
        arena_free(ar);
        ar = NULL;
    } else {
        // add the block to its free list.
        // done.
    }
    release_spinlock(&d->lock);
}

static void add_blocks(struct desc *d)
{
    ASSERT(d != NULL);
    void *pg = palloc_get();
    increment_rc(&kalloc_page_cnt);
    ASSERT(pg != NULL);

    /* Build an arena */
    struct arena *a = pg;
    a->magic = ARENA_MAGIC;
    a->nfr = d->bpa;
    a->desc = d;

    /* Put the rest of blocks on the free list. */
    pg += sizeof(struct arena);
    struct list_elem *elem;

    for (uint32_t i = 0; i < d->bpa; i++) {
        elem = pg;
        list_push_back(&d->flst, elem);
        // advance.
        pg += d->bsz;
    }
}

static inline struct arena *block2arena(void *blk)
{
    struct arena *ar = pg_round_down(blk);
    ASSERT(ar != NULL);
    if (ar->magic != ARENA_MAGIC) {
        PANIC("not an arena");
    }
    return ar;
}

static void arena_free(struct arena *a)
{
    ASSERT(a != NULL);
    if (a->magic != ARENA_MAGIC) {
        PANIC("not an arena");
    }

    // descriptor
    struct desc *d = a->desc;
    ASSERT(a->nfr == d->bpa);

    // remove all blocks
    void *pg = (void *)a;
    pg += sizeof(struct arena);
    struct list_elem *elem;
    for (uint32_t i = 0; i < d->bpa; i++) {
        elem = pg;
        list_remove(elem);
        // advance.
        pg += d->bsz;
    }

    palloc_free(a);
    decrement_rc(&kalloc_page_cnt);
}
