#include <fdutil/bitmap.h>
#include <common/defines.h>
#include <common/debug.h>
#include "test_util.h"
#include "test.h"

static char buf[256];

/** bitmap stress test: allocate bits of same number and frees.
 * @param bm a free bitmap, i.e. no bits allocated
 * @param batch number of bits allocated each time.
 */
static void stress_tester(struct bitmap *bm, long batch);

/**< ad-hoc tester */
static void adhoc_tester(struct bitmap *bm);

void bitmap_test(void)
{
    TEST_START;

    // initialize the bitmap
    struct bitmap bm;
    const long bits = sizeof(buf) * 0x8;
    bitmap_init(&bm, bits, (void *)buf);
    ASSERT(Memchk((const unsigned char *)buf, sizeof(buf), 0x0));

    // allocate very large number of bits,
    // which should fail.
    ASSERT(bitmap_alloc(&bm, bits * 2) == BITMAP_ERROR);

    // allocate all bits
    long ba = 0; // allocated bits
    long p;
    while ((p = bitmap_alloc(&bm, 1)) != BITMAP_ERROR) {
        ba++;
        ASSERT(bitmap_get(&bm, p));
    }
    // check that all bits is allocated
    ASSERT(ba == bits);

    // free all bits
    while (ba > 0L) {
        bitmap_free(&bm, --ba, 1);
        ASSERT(!bitmap_get(&bm, ba));
    }

    // allocate bits again to check leak
    while ((p = bitmap_alloc(&bm, 1)) != BITMAP_ERROR) {
        ba++;
        ASSERT(bitmap_get(&bm, p));
    }
    ASSERT(ba == bits);

    // free all bits
    while (ba > 0L) {
        bitmap_free(&bm, --ba, 1);
        ASSERT(!bitmap_get(&bm, ba));
    }

    // now run stress tests
    stress_tester(&bm, 2);
    stress_tester(&bm, 4);
    stress_tester(&bm, 8);
    stress_tester(&bm, 64);
    stress_tester(&bm, 256);

    // some weired number of bits
    // can your impl handle correctly?
    stress_tester(&bm, 3);
    stress_tester(&bm, 5);
    stress_tester(&bm, 7);
    stress_tester(&bm, 11);
    stress_tester(&bm, 13);
    stress_tester(&bm, 17);
    stress_tester(&bm, 23);
    stress_tester(&bm, 26);

    // what if we do some ad-hoc testing?
    ASSERT(Memchk((unsigned char *)buf, sizeof(buf), 0));
    adhoc_tester(&bm);
    ASSERT(Memchk((unsigned char *)buf, sizeof(buf), 0));

    // OK
    TEST_END;
}

static void stress_tester(struct bitmap *bm, long batch)
{
    const long bits = bm->bits;

    // allocate all bits
    long ba = 0; // allocated bits
    long p;
    while ((p = bitmap_alloc(bm, batch)) != BITMAP_ERROR) {
        ba++;

        // check that bits are correctly set.
        // note that p is the start of allocated bits.
        for (long i = 0; i < batch; i++) {
            ASSERT(bitmap_get(bm, p + i));
        }
    }

    // check that all bits is allocated
    // if a single bit is allocated more than once,
    // ba will (probably) not equal to bits / batch.
    ASSERT(ba == bits / batch);

    // now free all stuff.
    // by allocation algorithm, bit is allocated from
    // start to end.
    p = 0;
    for (long i = 0; i < bits / batch; i++) {
        bitmap_free(bm, p, batch);
        // check that bits are correctly set.
        for (long i = 0; i < batch; i++) {
            ASSERT(!bitmap_get(bm, p + i));
        }
        p += batch;
    }

    // ok.
}

#include "range.h"
static void adhoc_tester(struct bitmap *bm)
{
    // ad-hoc tester keeps generate random request
    // for allocating/freeing continuous bits.
    // this may trigger some assertion failure.

// number of ranges
#define NRG 64
// number of operations
#define NOP 1000000L
    static struct range rgs[NRG];
    // operation code(1: free, 0, 2: alloc)
    unsigned op;
    // # bits to be allocated
    unsigned bits;
    // position
    unsigned pos;

#define bitmap_test_multiple(p, val)                   \
    do {                                               \
        st = (long)rgs[p].start;                       \
        for (long j = 0; j < (long)rgs[p].size; j++) { \
            if (val) {                                 \
                ASSERT(bitmap_get(bm, st + j));        \
            } else {                                   \
                ASSERT(!bitmap_get(bm, st + j));       \
            }                                          \
        }                                              \
    } while (0)

#define find_range(val)                      \
    do {                                     \
        pos = NRG;                           \
        for (unsigned t = 0; t < pos; t++) { \
            if (rgs[t].valid == val) {       \
                pos = t;                     \
            }                                \
        }                                    \
    } while (0)

#define not_found     \
    if (pos == NRG) { \
        continue;     \
    }

    for (long c = 0; c < NOP; c++) {
        op = rand() % 3;
        bits = rand() % 255 + 1;

        if (op == 1) {
            // find and free a slot
            find_range(1);

            // no available slot
            not_found

                    // free and test-free.
                    long st = (long)rgs[pos].start;
            bitmap_free(bm, st, rgs[pos].size);
            bitmap_test_multiple(pos, 0);

            // clear valid bit
            rgs[pos].valid = 0;
        } else {
            // find and fill a slot
            find_range(0);

            // no available slot
            not_found

                    // allocate and check.
                    rgs[pos]
                            .start = (void *)bitmap_alloc(bm, bits);
            long st = (long)rgs[pos].start;
            if (st != BITMAP_ERROR) {
                // set valid bit
                rgs[pos].valid = 1;
                rgs[pos].size = bits;

                // check intersection
                int ret = rg_find((struct range *)rgs, &rgs[pos], NRG);
                if (ret != RG_ERR) {
                    LOG("Fatal error: intersection detected");
                    for (;;)
                        ;
                }
                // check bits
                bitmap_test_multiple(pos, 1);
            }
        }
    }

    // free all bits
    for (int i = 0; i < NRG; i++) {
        if (rgs[i].valid) {
            long st = (long)rgs[i].start;

            // free and test-free.
            bitmap_free(bm, st, rgs[i].size);
            bitmap_test_multiple(i, 0);

            // clear valid bit
            rgs[i].valid = 0;
        }
    }

    // ok.
}

// now run bitmap test.
void run_test(void)
{
    // this is a single core test,
    // i.e. not concurrent
    if (cpuid() == 0)
        bitmap_test();
}
