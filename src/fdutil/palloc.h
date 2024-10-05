#pragma once
#ifndef __FDUTIL_PALLOC_
#define __FDUTIL_PALLOC_

#include "stddef.h"
#include <aarch64/mmu.h>
#undef KSPACE_MASK

#define PGSIZE PAGE_SIZE

/** returns page offset */
static inline size_t pg_off(void *addr)
{
    size_t ret = (size_t)addr;
    return ret & 0xfff;
}

/** returns aligned address(down) */
static inline void *pg_round_down(void *addr)
{
    return addr - pg_off(addr);
}

/** returns aliged address(up) */
static inline void *pg_round_up(void *addr)
{
    addr += PGSIZE - 1;
    return addr - pg_off(addr);
}

/** Initialize palloc module */
void palloc_init(void);

/** Initialize with limited number of pages(for testing).
 * @param npgs number of pages on each cpu.
 */
void palloc_init_limit(int *npgs);

/** Get a single page. */
void *palloc_get(void);

/** Free a page. */
void palloc_free(void *pg);

/** Currently number of pages allocated. */
size_t palloc_used(void);

/** Interface of an page allocator */
struct palloc_intf {
    void *(*get)(); // get a single page
    void (*free)(void *); // free a single page
    void *(*getmult)(unsigned int); // get multiple pages(optional)
    void (*freemult)(void *, unsigned int); // free multiple(optional)
};

#endif // __FDUTIL_PALLOC_