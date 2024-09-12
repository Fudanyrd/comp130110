#pragma once
/**
 * Malloc/free implementation.
 * Amortized cost is O(1).
 */
#ifndef __FDUTIL_MALLOC_
#define __FDUTIL_MALLOC_

#include "palloc.h"
#include "stddef.h"
#include <common/spinlock.h>

void malloc_init(void);
void *malloc(size_t nb);
void free(void *pt);

#endif // __FDUTIL_MALLOC_
