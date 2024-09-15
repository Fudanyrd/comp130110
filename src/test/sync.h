#pragma once
/**
 * Sync.h: synchronize all threads(cpus).
 * 
 * Typical usage: call sync_init() in test_init(),
 * and call sync_record() in run_test() and should
 * be done BEFORE using sync functionality.
 */
#ifndef __TEST_SYNC_
#define __TEST_SYNC_

#include <fdutil/stddef.h>

void sync_init(void);
void sync_record(void);

/** Join all running threads */
void sync(size_t cnt);

#endif // __TEST_SYNC_
