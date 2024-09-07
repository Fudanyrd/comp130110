#include <fdutil/bitmap.h>
#include <common/defines.h>
#include <common/debug.h>
#include "test_util.h"

static char buf[256];

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

    // OK
    TEST_END;
}
