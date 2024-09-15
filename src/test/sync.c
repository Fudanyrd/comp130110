#include "sync.h"
#include <aarch64/intrinsic.h>
#include <common/spinlock.h>
#include <common/rc.h>
#include <aarch64/intrinsic.h>

static SpinLock sync_lk;
static volatile size_t counter;
static volatile size_t ncpu;

void sync_init(void)
{
    counter = 0;
    ncpu = 0;
    init_spinlock(&sync_lk);
}

/** Record number of cpus started */
void sync_record()
{
    __atomic_fetch_add(&ncpu, 1, __ATOMIC_ACQ_REL);
}

void sync(size_t cnt)
{
    arch_dsb_sy();
    __atomic_fetch_add(&counter, 1, __ATOMIC_ACQ_REL);

    for (;;) {
        if (counter == cnt * ncpu) {
            break;
        }
    }
    arch_dsb_sy();
}
