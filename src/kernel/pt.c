#include <kernel/pt.h>
#include <kernel/mem.h>
#include <common/string.h>
#include <aarch64/intrinsic.h>
#include <aarch64/mmu.h>

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
    u64 ret = VA_PART0(va);
    ASSERT(ret < N_PTE_PER_TABLE);
    return ret;
}
/** Returns the level 1 table index */
static inline u64 pte_idx_lv1(u64 va) {
    // bit [38:30]
    u64 ret = VA_PART1(va);
    ASSERT(ret < N_PTE_PER_TABLE);
    return ret;
}

/** Returns the level 2 table index */
static inline u64 pte_idx_lv2(u64 va) {
    // bit [29:21]
    u64 ret = VA_PART2(va);
    ASSERT(ret < N_PTE_PER_TABLE);
    return ret;
}
/** Returns the level 3 table index */
static inline u64 pte_idx_lv3(u64 va) {
    // bit [20:12]
    u64 ret = VA_PART3(va);
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
    PTEntry *ptlv0 = (PTEntry *)(pgdir->pt[lv0] & (~PTE_TABLE));

    // at level 1, bit [38:30]
    if (ptlv0 == NULL) {
        if (alloc) {
            ptlv0 = pte_page();
            pgdir->pt[lv0] = K2P(ptlv0) | PTE_TABLE;
        } else {
            return NULL;
        }
    } else {
        // convert to virtual address
        ptlv0 = (PTEntry *)(P2K(ptlv0) & (~PTE_TABLE));
    }
    const u64 lv1 = pte_idx_lv1(va);
    PTEntry *ptlv1 = (PTEntry *)ptlv0[lv1];

    // at level 2, bit [29:21]
    if (ptlv1 == NULL) {
        if (alloc) {
            ptlv1 = pte_page();
            ptlv0[lv1] = K2P(ptlv1) | PTE_TABLE;
        } else {
            return NULL;
        }
    } else {
        ptlv1 = (PTEntry *)(P2K(ptlv1) & (~PTE_TABLE));
    }
    const u64 lv2 = pte_idx_lv2(va);
    PTEntry *ptlv2 = (PTEntry *) ptlv1[lv2];

    // at level 3, bit [20:12]
    if (ptlv2 == NULL) {
        if (alloc) {
            ptlv2 = pte_page();
            ptlv1[lv2] = K2P(ptlv2) | PTE_TABLE;
        } else {
            return NULL;
        }
    } else {
        ptlv2 = (PTEntry *)(P2K(ptlv2) & (~PTE_TABLE));
    }
    const u64 lv3 = pte_idx_lv3(va);
    return &ptlv2[lv3];
}

void init_pgdir(struct pgdir *pgdir)
{
    pgdir->pt = NULL;
}

/** Free a page table at level lv
 * @param pte kernel address to a page used by page table
 * @param lv level in the page table, range from top(0) to bottom(3).
 */
static void pgdir_free_lv(PTEntry *pte, int lv)
{
    ASSERT(lv >= 0 && lv < 4);
    if (pte == NULL) {
        return;
    }
    // check page offset
    ASSERT(((u64)pte & 0xFFF) == 0);

    if (lv == 3) {
        // last level of page table
        kfree_page(pte);
    } else {
        // at this level, free the page at next level,
        // and then the page itself.
        for (int i = 0; i < N_PTE_PER_TABLE; i++) {
            if (pte[i] != 0x0) {
                pgdir_free_lv((PTEntry *)(P2K(pte[i]) & (~PTE_TABLE)), lv + 1);
            }
        }
        kfree_page(pte);
    }
}

void free_pgdir(struct pgdir *pgdir)
{
    // TODO:
    // Free pages used by the page table. If pgdir->pt=NULL, do nothing.
    // DONT FREE PAGES DESCRIBED BY THE PAGE TABLE
    if (pgdir->pt == NULL) {
        return;
    }
    // recursively free all pages used by page table
    pgdir_free_lv(pgdir->pt, 0);
    pgdir->pt = NULL;
}

void attach_pgdir(struct pgdir *pgdir)
{
    extern PTEntries invalid_pt;
    if (pgdir->pt)
        arch_set_ttbr0(K2P(pgdir->pt));
    else
        arch_set_ttbr0(K2P(&invalid_pt));
}
