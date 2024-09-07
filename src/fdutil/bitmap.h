#pragma once
#ifndef __FDUTIL_BITMAP_
#define __FDUTIL_BITMAP_

#define BITMAP_ERROR (-1L)

/** bitmap struct 
 * Notice that we use `long` type to denote
 * bits. This allows creation of 2^63 bits.
 */
struct bitmap {
    long bits; // n bits to alloc/free
    unsigned int *buf; // buffer of the bitmap
};

/** Initialize the bitmap. 
 * @param bits number of bits of map
 * @param buf user should provide a buffer
 */
void bitmap_init(struct bitmap *bm, long bits, void *buf);

/** Allocate continuous bits.
 * @param n number of bits needed
 * @return BITMAP_ERROR if allocation failure
 */
long bitmap_alloc(struct bitmap *bm, long n);

/** Free continuous bits.
 * @param st bits allocated via bitmap_alloc.
 * @param n number of bits to be freed.
 */
void bitmap_free(struct bitmap *bm, long st, long n);

/** 
 * @return 0 if bit at pos is 0.
 */
int bitmap_get(struct bitmap *bm, long pos);

#endif // __FDUTIL_BITMAP_
