#pragma once
/**
 * Sync.h: synchronize all threads(cpus).
 * 
 * Typical usage: call sync_init() in test_init().
 */
#ifndef __TEST_SYNC_
#define __TEST_SYNC_

#include <fdutil/stddef.h>

void sync_init(void);

/** Join all running threads */
void sync(size_t cnt);

#endif // __TEST_SYNC_
