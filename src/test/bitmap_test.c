#include <fdutil/bitmap.h>
#include <common/defines.h>
#include <common/debug.h>
#include "test_util.h"

static char buf[256];

/** bitmap stress test: allocate bits of same number and frees.
 * @param bm a free bitmap, i.e. no bits allocated
 * @param batch number of bits allocated each time.
 */
static void stress_tester(struct bitmap *bm, long batch);

void bitmap_test(void) {
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
    long ba = 0;  // allocated bits
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

    // OK
    TEST_END;
}

static void stress_tester(struct bitmap *bm, long batch) {
    const long bits = bm->bits;

    // allocate all bits
    long ba = 0;  // allocated bits
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
