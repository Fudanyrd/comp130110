#include "malloc.h"
#include "palloc.h"
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

/** Add a page of blocks to descriptor d. 
 * @return 1 if success, 0 if page cannot allocate.
 */
static int add_blocks(struct desc *d);

/** An arena */
struct arena {
    struct desc *desc; // descriptor
    uint32_t nfr; // number of free block
    uint32_t magic; // magic number
};

/** Sizes of blocks to reduce page usage */
static const int szset[] = { 16,
                             32,
                             48,
                             64,
                             72,
                             80,
                             96,
                             128,
                             144,
                             208,
                             288,
                             408,
                             576,
                             1016,
                             1360,
                             2040,
                             PGSIZE - sizeof(struct arena) };

// note that organizing so many descriptors is error-prone.
// (No, I don't mean that my descriptor implementation has bug)
// it is sugguested to change the array to simpler ones, for example,
// { 16, 32, 64, 128, 256, 512, 1024, 2048 };

/** Number of descriptors */
#define NDESC ((int)(sizeof(szset) / sizeof(szset[0])))

/** Returns the arena managing the blk. */
static inline struct arena *block2arena(void *blk);

/** Free the space of an arena */
static void arena_free(struct arena *a);

/** Descriptors(private) */
static struct desc descs[NDESC];

/** Arena magic number */
#define ARENA_MAGIC 0x9a548eed

/** Page allocator interface */
static struct palloc_intf pintf;

void malloc_init(void)
{
    STATIC_ASSERT(sizeof(struct arena) % 8 == 0);
    // initialize 8 descriptors, each handles memory
    // block of size:
    // 16, 32, 64, 128, 256
    // 512, 1024, 2048

    uint32_t sz;
    for (int i = 0; i < NDESC; i++) {
        sz = szset[i];
        if (sz % 8 != 0) {
            PANIC("Alignment");
        }
        if (sz + sizeof(struct arena) > PGSIZE) {
            PANIC("overflow");
        }
        descs[i].bsz = sz;
        descs[i].bpa = (PGSIZE - sizeof(struct arena)) / sz;
        init_spinlock(&descs[i].lock);
        list_init(&descs[i].flst);
        ASSERT(list_empty(&descs[i].flst));
    }

    /** Directly use pallocator interface */
    pintf.get = palloc_get;
    pintf.free = palloc_free;
    /** Do not support alloc/free multiple pages */
    pintf.getmult = NULL;
    pintf.freemult = NULL;
}

/** Initialize malloc with a different allocator 
 * @param intf a thread-safe pallocator interface
 */
void malloc_mod_intf(struct palloc_intf intf)
{
    ASSERT(intf.get != NULL && intf.free != NULL);
    pintf = intf;
}

/** Allocate a block of size 2^i, 
 * where i is the smallest integer satisfying 2^i >= nb.
 */
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
        if (add_blocks(d) == 0) {
            // allocation failure
            // release the lock, return null.
            release_spinlock(&d->lock);
            return NULL;
        }
    }
    ASSERT(!list_empty(&d->flst));

    ret = (void *)list_pop_front(&d->flst);
    struct arena *ar = block2arena(ret);
    ASSERT(ar->nfr > 0);
    ar->nfr--;

    release_spinlock(&d->lock);
    return ret;
}

/** Free a block allocated by malloc. */
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

static int add_blocks(struct desc *d)
{
    ASSERT(d != NULL);
    void *pg = pintf.get();
    /** If pg is null or max allocation exceed limit */
    if (pg == NULL) {
        return 0;
    }
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

    // success
    return 1;
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

    pintf.free(a);
    decrement_rc(&kalloc_page_cnt);
}
