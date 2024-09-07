#include "bitmap.h"
#include "arithm.h"
#include <common/defines.h>
#include <common/debug.h>

/**
 * Specifications:
 * - all helper methods(those declared static) does not have
 *   to check nullptr and array out of bound.
 * - all API methods should check nullptr and other invalid 
 *   parameters.
 */

/** element type should be unsigned */
typedef unsigned int elem_type;

#define BITS_PER_ELEM (sizeof(elem_type) * 0x8)

static inline long pos2elem(long p)
{
    return p / BITS_PER_ELEM;
}

static inline long pos2off(long p)
{
    return p % BITS_PER_ELEM;
}
#define BITMAP_GET(bm, pos) \
    ((bm->buf[pos2elem(pos)] & (1 << pos2off(pos))) != 0)

static inline void bitmap_set(struct bitmap *bm, long p, int flag)
{
    if (flag == 0) {
        // zero out the bit
        bm->buf[pos2elem(p)] &= (~(1 << pos2off(p)));
    } else {
        bm->buf[pos2elem(p)] |= (1 << pos2off(p));
    }
}

static inline void bitmap_test_and_set(struct bitmap *bm, long p, int flag)
{
    if (flag == 0) {
        ASSERT(bitmap_get(bm, p));
        bm->buf[pos2elem(p)] &= (~(1 << pos2off(p)));
    } else {
        ASSERT(!bitmap_get(bm, p));
        bm->buf[pos2elem(p)] |= (1 << pos2off(p));
    }
}

void bitmap_init(struct bitmap *bm, long bits, void *buf)
{
    ASSERT(bm != NULL);
    ASSERT(buf != NULL);
    ASSERT(bits > 0L);

    bm->bits = bits;
    bm->buf = buf;

    const unsigned long bufsz = ROUND_UP(bits, BITS_PER_ELEM);
    // zero out the buffer.
    for (unsigned long i = 0; i < bufsz; i++) {
        bm->buf[i] = 0x0;
    }
}

int bitmap_get(struct bitmap *bm, long pos)
{
    ASSERT(bm != NULL && bm->bits < pos);
    return BITMAP_GET(bm, pos);
}

long bitmap_alloc(struct bitmap *bm, long n)
{
    ASSERT(bm != NULL);
    /* Should fail even if just enough space can be allocated. */
    if (n >= bm->bits) {
        return BITMAP_ERROR;
    }

    long k;

    /**
     * Here i use two iterators. At each iteration,
     * I check whether there's bits colored 1. If so,
     * the inner loop shall terminate.
     */
    for (long i = 0; i + n < bm->bits; i++) {
        k = i;
        for (; k < bm->bits; k++) {
            if (BITMAP_GET(bm, k)) {
                break;
            }
        }

        if (k - n >= i) {
            ASSERT(k - n == i);
            return i;
        }
        i = k + 1;
    }

    return BITMAP_ERROR;
}

void bitmap_free(struct bitmap *bm, long st, long n)
{
    ASSERT(bm != NULL);
    ASSERT(n > 0 && st >= 0);

    // n is now end point.
    n += st;
    ASSERT(n < bm->bits);

    // mark all bits as zero.
    for (long i = st; i < n; i++) {
        bitmap_test_and_set(bm, i, 0);
    }
}
