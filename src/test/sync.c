#include "sync.h"
#include <aarch64/intrinsic.h>
#include <common/spinlock.h>

static SpinLock sync_lk;
static volatile size_t counter;

void sync_init(void)
{
    counter = 0;
    init_spinlock(&sync_lk);
}

void sync(size_t cnt)
{
    acquire_spinlock(&sync_lk);
    counter++;
    release_spinlock(&sync_lk);

    for (;;) {
        acquire_spinlock(&sync_lk);
        if (counter == cnt * 4) {
            release_spinlock(&sync_lk);
            break;
        }
        release_spinlock(&sync_lk);
    }
}
