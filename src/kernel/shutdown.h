#pragma once
#ifndef __KERNEL_SHUTDOWN_
#define __KERNEL_SHUTDOWN_

/**
 * A cpu that calls shut down may not be shutdown immediately.
 * Rather, it should wait for all cpu to finish. We do this 
 * through a counter and a spinlock.
 * 
 * A correct use of shutdown module may be:
 * - For cpu0: call shut_init, shut_record, and shutdown.
 * - For other cpus: call shut_record and then shutdown.
 */

/** Shutdown the cpu. */
void shutdown(void);

/** Initialize */
void shut_init(void);

/** Record a running cpu. */
void shut_record(void);

#endif // __KERNEL_SHUTDOWN_
