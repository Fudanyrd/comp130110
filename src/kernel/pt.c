#include <kernel/pt.h>
#include <kernel/mem.h>
#include <common/string.h>
#include <aarch64/intrinsic.h>

/** Returns a page filled with 0 */
static inline void *pte_page(void) {
    void *ret = kalloc_page();
    ASSERT(ret != NULL);
    memset(ret, 0, PAGE_SIZE);
    return ret;
}

/** Returns the level 0 table index */
static inline u64 pte_idx_lv0(u64 va) {
    // bit [47:39]
    u64 ret = va >> 39;
    ret &= (N_PTE_PER_TABLE - 1);
    ASSERT(ret < N_PTE_PER_TABLE);
    return ret;
}
/** Returns the level 1 table index */
static inline u64 pte_idx_lv1(u64 va) {
    // bit [47:39]
    u64 ret = va >> 39;
    ret &= (N_PTE_PER_TABLE - 1);
    ASSERT(ret < N_PTE_PER_TABLE);
    return ret;
}

PTEntriesPtr get_pte(struct pgdir *pgdir, u64 va, bool alloc)
{
    // TODO:
    // Return a pointer to the PTE (Page Table Entry) for virtual address 'va'
    // If the entry not exists (NEEDN'T BE VALID), allocate it if alloc=true, or return NULL if false.
    // THIS ROUTINUE GETS THE PTE, NOT THE PAGE DESCRIBED BY PTE.
    ASSERT(pgdir != NULL);

    // at each level, based on alloc is true or not, 
    // do the following: allocate page for this level,
    // and compute the index to the next level.

    // at level 0, bit [47:39]
    if (pgdir->pt == NULL) {
        if (alloc) {
            pgdir->pt = pte_page();
        } else {
            return NULL;
        }
    }
    const u64 lv0 = pte_idx_lv0(va);
    PTEntry *ptlv0 = (PTEntry *)pgdir->pt[lv0];

    // at level 1, bit [38:30]
    if (ptlv0 == NULL) {
        if (alloc) {
            pgdir->pt[lv0] = pte_page();
            ptlv0 = (PTEntry *)pgdir->pt[lv0];
        } else {
            return NULL;
        }
    }
    const u64 lv1 = pte_idx_lv1(va);
    PTEntry *ptlv1 = (PTEntry *)ptlv0[lv1];

    // at level 2, bit [29:21]
}

void init_pgdir(struct pgdir *pgdir)
{
    pgdir->pt = NULL;
}

void free_pgdir(struct pgdir *pgdir)
{
    // TODO:
    // Free pages used by the page table. If pgdir->pt=NULL, do nothing.
    // DONT FREE PAGES DESCRIBED BY THE PAGE TABLE
}

void attach_pgdir(struct pgdir *pgdir)
{
    extern PTEntries invalid_pt;
    if (pgdir->pt)
        arch_set_ttbr0(K2P(pgdir->pt));
    else
        arch_set_ttbr0(K2P(&invalid_pt));
}
