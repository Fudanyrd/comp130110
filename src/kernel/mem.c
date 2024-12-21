#include <aarch64/mmu.h>
#include <common/rc.h>
#include <common/spinlock.h>
#include <driver/memlayout.h>
#include <kernel/mem.h>
#include <common/string.h>
// #include <kernel/printk.h>

#include <fdutil/malloc.h>

RefCount kalloc_page_cnt;

static void *zero_page;

void kinit()
{
    init_rc(&kalloc_page_cnt);
    /** Initialize palloc and malloc module. */
    palloc_init();
    malloc_init();

    zero_page = palloc_get();
    ASSERT(zero_page != NULL);
    memset(zero_page, 0, PAGE_SIZE);
}

void *kalloc_page()
{
    increment_rc(&kalloc_page_cnt);

    return palloc_get();
}

void kfree_page(void *p)
{
    decrement_rc(&kalloc_page_cnt);
    // check offset
    ASSERT(((u64)p & 0xffful) == 0);

    if (p == zero_page) {
        // do not do free on this page.
        return;
    }
    palloc_free(p);
    return;
}

void *kalloc(unsigned long long size)
{
    /** Note to TAs: malloc and free will call increment_rc
     * and decrement_rc to ensure that page usage 
     * are correctly collected. In src/fdutil/malloc.c
     * line 135 and 189.
     */
    return malloc(size);
}

void kfree(void *ptr)
{
    free(ptr);
    return;
}

void *kshare_page(void *pg)
{
    return palloc_share(pg);
}

void *kalloc_zero()
{
    increment_rc(&kalloc_page_cnt);
    return zero_page;
}
