#include "test.h"
#include "test_util.h"
#include "range.h"

/**
 * Integrated test for lab1: will test implementation of
 * list, bitmap and malloc. Good luck!
 */

#include <fdutil/lst.h>
#include <fdutil/bitmap.h>
#include <fdutil/malloc.h>

#define NALLOC 10000
#define BUFSZ 512
#define MAGIC 0x1234fedc

/** range element */
struct rg_elem {
    struct range rg; // range in bitmap
    struct list_elem elem; // lst elem
};

static struct list lst;
static struct bitmap bm;

// free a rg_elem struct
static void rgbm_free()
{
    struct rg_elem *re = NULL;
    struct list_elem *e = NULL;
    if (!list_empty(&lst)) {
        e = list_pop_front(&lst);
        re = list_entry(e, struct rg_elem, elem);
        // validate list element
        ASSERT(&re->elem == e);

        // not an valid allocation
        if (re->rg.valid == 0) {
            free(re);
            return;
        }

        ASSERT(re->rg.valid == 0x1);
        bitmap_free(&bm, re->rg.start, re->rg.size);
        for (long it = re->rg.start; it < re->rg.start + re->rg.size; it++) {
            ASSERT(!bitmap_get(&bm, it));
        }
        free(re);
    }
    return;
}

// allocate a rg_elem struct,
// and bitmap bits
static void rgbm_alloc()
{
    struct rg_elem *re = malloc(sizeof(struct rg_elem));
    ASSERT(re != NULL);
    re->rg.valid = 0x0;

    int b = rand() % 256;
    long ret = bitmap_alloc(&bm, b);
    if (ret != BITMAP_ERROR) {
        re->rg.valid = 0x1;
        re->rg.start = (void *)ret;
        re->rg.size = b;
        for (long it = re->rg.start; it < re->rg.start + re->rg.size; it++) {
            ASSERT(bitmap_get(&bm, it));
        }
    } else {
        // no need to check intersection
        list_push_back(&lst, &re->elem);
        return;
    }

    // check intersection
    struct list_elem *e;
    for (e = list_begin(&lst); e != list_end(&lst); e = list_next(e)) {
        struct rg_elem *cur = list_entry(e, struct rg_elem, elem);
        if (rg_intersect(&re->rg, &cur->rg)) {
            PANIC("intersection detected!");
        }
    }

    // push to back of list, done.
    list_push_back(&lst, &re->elem);
    return;
}

void run_test()
{
    TEST_START;
    // initialization: list
    list_init(&lst);
    ASSERT(list_empty(&lst));

    // initialization: bitmap
    void *buf = malloc(BUFSZ);
    memset(buf, 0xfd, BUFSZ);
    ASSERT(buf != NULL);
    bitmap_init(&bm, BUFSZ - sizeof(int), buf);
    ASSERT(Memchk(buf, BUFSZ - sizeof(int), 0));

    // initialization: magic pointer
    int *const mgc = buf + (BUFSZ - sizeof(int));
    *mgc = MAGIC;

    for (long c = 0; c < NALLOC; c++) {
        // operation: 0, 2: allocate, 1: free
        int op = rand() % 3;

        if (op == 1) {
            // free
        } else {
        }
    }

    ASSERT(list_size(&lst) == NALLOC);
    while (!list_empty(&lst)) {
    }

    // congrats!
    free(buf);
    TEST_END;
}

void test_init()
{
}
