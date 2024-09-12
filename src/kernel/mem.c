#include <aarch64/mmu.h>
#include <common/rc.h>
#include <common/spinlock.h>
#include <driver/memlayout.h>
#include <kernel/mem.h>
// #include <kernel/printk.h>

#include <fdutil/malloc.h>

RefCount kalloc_page_cnt;

void kinit()
{
    init_rc(&kalloc_page_cnt);
    /** Initialize palloc and malloc module. */
    palloc_init();
    malloc_init();
}

void *kalloc_page()
{
    increment_rc(&kalloc_page_cnt);

    return palloc_get();
}

void kfree_page(void *p)
{
    decrement_rc(&kalloc_page_cnt);
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
