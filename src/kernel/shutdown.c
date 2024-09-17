#include "shutdown.h"
#include <aarch64/intrinsic.h>
#include <common/spinlock.h>
#include <common/debug.h>
#include "printk.h"

static SpinLock sd_lock;
static volatile int ncpu;
static volatile int all;

#define Lock(lk) acquire_spinlock(&lk)
#define Unlock(lk) release_spinlock(&lk)

void shutdown(void)
{
    // atomic sub
    Lock(sd_lock);
    ASSERT(ncpu > 0);
    ncpu--;
    Unlock(sd_lock);

    if (cpuid() == 0) {
        for (;;) {
            // automic read
            Lock(sd_lock);
            if (ncpu == 0) {
                // poweroff, this will cause qemu to exit
                psci_fn(PSCI_SYSTEM_OFF, 0, SECONDARY_CORE_ENTRY, 0);
                printk("%s\n", "still running...");
            }
            Unlock(sd_lock);
        }
        // should not reach here
    } else {
        for (;;) {
            // just wait to be shutdown...
            asm volatile("wfe");
        }
    }
}

void shut_init(void)
{
    ncpu = 0;
    init_spinlock(&sd_lock);
}

void shut_record(void)
{
    Lock(sd_lock);
    ncpu++;
    all = ncpu;
    Unlock(sd_lock);
}

int ncpu_all()
{
    return all;
}

int ncpu_run()
{
    return ncpu;
}
