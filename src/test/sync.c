#include "sync.h"
#include <aarch64/intrinsic.h>
#include <common/spinlock.h>
#include <common/rc.h>
#include <common/debug.h>
#include <kernel/shutdown.h>
#include <aarch64/intrinsic.h>

static RefCount counter;

void sync_init(void)
{
    init_rc(&counter);
}

void sync(size_t cnt)
{
    arch_dsb_sy();
    increment_rc(&counter);

    for (;;) {
        ASSERT((size_t)counter.count <= cnt * ncpu_all());
        if ((size_t)counter.count == cnt * ncpu_all()) {
            break;
        }
    }
    arch_dsb_sy();
}
