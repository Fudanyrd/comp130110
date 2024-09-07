#pragma once
#ifndef __TEST_RANGE_
#define __TEST_RANGE_

#include <common/defines.h>
#include <common/debug.h>

#define RG_ERR (-1)

/**< Range struct. Useful for testing the correctness of
 * memory/page allocators, where intersections should be 
 * checked.
 */
struct range {
    void *start; // start address of range(inclusive)
    unsigned int size; // size of the range(exclusive)
    int valid; // valid bit
};

static void *rg_end(struct range *r) __attribute__((unused));
static int rg_intersect(struct range *r1, struct range *r2)
        __attribute__((unused));

/** Returns the ending address of a range. */
static void *rg_end(struct range *r)
{
    return r->start + r->size;
}

/** Returns 1 if range r1 and r2 intersects. */
static int rg_intersect(struct range *r1, struct range *r2)
{
    // clang-format off
    /**
     * Intersected range has the following two cases:
     * [    [   )   )
     * r1   r2  e1  e2, i.e. r2\in [r1, e1)
     * 
     * Or:
     * [    [   )   )
     * r2   r1  e2  e1, i.e. r1\in [r2, e2)
     */
    // clang-format on
    ASSERT(r1 != NULL && r2 != NULL);
    void *end1 = rg_end(r1);
    void *end2 = rg_end(r2);
    return (r2->start < end1 && r2->start >= r1->start) ||
           (r1->start < end2 && r1->start >= r2->start);
}

/** Returns 1 if no intersection within [begin, end) 
 * NOTE that only those with valid bit set will be checked. 
 */
int rg_safe(struct range *rpt, int begin, int end);

/** Find intersection in an array.
 * @param rarr: array of ranges
 * @param ro: range object to find intersection
 * @param size: size of the range array
 * @return the position of intersection, RG_ERR if not detected
 */
int rg_find(struct range *rarr, struct range *ro, int size);

#endif // __TEST_RANGE_
