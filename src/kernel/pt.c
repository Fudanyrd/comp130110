#include <kernel/pt.h>
#include <kernel/mem.h>
#include <common/string.h>
#include <aarch64/intrinsic.h>
#include <aarch64/mmu.h>
#include <kernel/exec1207.h>
#include <fs/file1206.h>
#include <kernel/mmap1217.h>

/** Returns a page filled with 0 */
static inline void *pte_page(void)
{
    void *ret = kalloc_page();
    ASSERT(ret != NULL);
    memset(ret, 0, PAGE_SIZE);
    return ret;
}

/** Returns the level 0 table index */
static inline u64 pte_idx_lv0(u64 va)
{
    // bit [47:39]
    u64 ret = VA_PART0(va);
    ASSERT(ret < N_PTE_PER_TABLE);
    return ret;
}
/** Returns the level 1 table index */
static inline u64 pte_idx_lv1(u64 va)
{
    // bit [38:30]
    u64 ret = VA_PART1(va);
    ASSERT(ret < N_PTE_PER_TABLE);
    return ret;
}

/** Returns the level 2 table index */
static inline u64 pte_idx_lv2(u64 va)
{
    // bit [29:21]
    u64 ret = VA_PART2(va);
    ASSERT(ret < N_PTE_PER_TABLE);
    return ret;
}
/** Returns the level 3 table index */
static inline u64 pte_idx_lv3(u64 va)
{
    // bit [20:12]
    u64 ret = VA_PART3(va);
    ASSERT(ret < N_PTE_PER_TABLE);
    return ret;
}

static PTEntry *pte_walk(PTEntry *table, u64 va, int lv, bool alloc)
{
    typedef u64 (*idx_fn)(u64);
    static idx_fn indices[4] = {
        pte_idx_lv0,
        pte_idx_lv1,
        pte_idx_lv2,
        pte_idx_lv3,
    };

    ASSERT(lv < 4);
    // check table is not null and page aligned.
    ASSERT(table != NULL);
    ASSERT(((u64)table & 0xfff) == 0);
    const u64 index = indices[lv](va);

    if (lv == 3) {
        return &table[index];
    }
    PTEntry *next = (PTEntry *)(P2K(table[index]) & (~PTE_TABLE));
    if (table[index] == 0x0) {
        if (alloc) {
            next = pte_page();
            table[index] = K2P(next) | PTE_TABLE;
        } else {
            return NULL;
        }
    }
    return pte_walk(next, va, lv + 1, alloc);
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
    return pte_walk(pgdir->pt, va, 0, alloc);
#ifdef DISCARDED
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
    PTEntry *ptlv2 = (PTEntry *)ptlv1[lv2];

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
#endif
}

void init_pgdir(struct pgdir *pgdir)
{
    pgdir->pt = NULL;
    // empty sections
    list_init(&pgdir->sections);
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

    // free the sections.
    struct list_elem *elem;
    while (!list_empty(&pgdir->sections)) {
        elem = list_pop_front(&pgdir->sections);
        struct section *sec = list_entry(elem, struct section, node);
        ASSERT(sec->start % PAGE_SIZE == 0);

        // FIXME: for writable files,
        // the content may have to be written back.
        section_unmap(pgdir, sec);
        kfree(sec);
    }

    // recursively free all pages used by page table
    pgdir_free_lv(pgdir->pt, 0);
    pgdir->heap = NULL;
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

static bool sec_less_func(const struct list_elem *a, const struct list_elem *b,
                          void *aux)
{
    struct section *sa = list_entry(a, struct section, node);
    struct section *sb = list_entry(b, struct section, node);
    ASSERT(sa->start % PAGE_SIZE == 0);
    ASSERT(sb->start % PAGE_SIZE == 0);
    return sa->start < sb->start;
}

void pgdir_add_section(struct pgdir *pgdir, struct section *sec)
{
    ASSERT(sec != NULL);
    // do not check intersection.
    list_insert_ordered(&pgdir->sections, &sec->node, sec_less_func, NULL);
}

// install the page at va in src to dst.
// based on whether the page is mutable,
// take different 'copy' approaches
static void page_copy(struct pgdir *dst, struct pgdir *src, u64 va, int flags)
{
    bool writable = (flags & PF_W) != 0;
    ASSERT(va % PAGE_SIZE == 0 && va != 0);
    // get the entry from src.
    PTEntry *sentr = get_pte(src, va, false);
    PTEntry *dentr = get_pte(dst, va, true);
    ASSERT(dentr != NULL);

    if (sentr == NULL) {
        // not mapped
        *dentr = 0;
        return;
    }
    if ((*sentr & PTE_VALID) == 0) {
        // not mapped
        *dentr = 0;
        return;
    }
    ASSERT((*sentr & PTE_PAGE) == PTE_PAGE);

    // COW: mark as readable, and share the pages.
    // this is done lazily, of course
    // later if write happens, mmap1217::section_install
    // will handle it.

    // first tell the allocator to incr the page count.
    void *shared = (void *)(P2K(*sentr & (~0xffful)));
    // page for dst pgdir.
    void *dup = kshare_page(shared);

    // if the page is shared.
    bool is_shared = (flags & PF_S) != 0;
    if (is_shared) {
        // just create the mapping.
        // don't care about anything else.
        *dentr = *sentr;
        return;
    }

    if (shared == dup) {
        *sentr |= PTE_RO;
        *dentr = *sentr;
    } else {
        *dentr = K2P(dup) | PTE_USER_DATA;
        if (!writable) {
            *dentr = *dentr | PTE_RO;
        }
    }
}

void pgdir_clone(struct pgdir *dst, struct pgdir *src)
{
    ASSERT(dst != NULL && src != NULL);
    struct list *l = &src->sections;
    struct list_elem *elem = list_begin(l);

    if (!list_empty(l)) {
        for (; elem != list_end(l); elem = list_next(elem)) {
            struct section *s = list_entry(elem, struct section, node);
            // make a clone of the node.
            struct section *sec = kalloc(sizeof(struct section));
            sec->flags = s->flags;
            sec->npages = s->npages;
            sec->start = s->start;
            if (s->flags & PF_F) {
                sec->fobj = fshare(s->fobj);
                sec->offset = s->offset;
            } else {
                sec->fobj = NULL;
            }

            // set the heap of dst.
            if (s == src->heap) {
                dst->heap = sec;
            }

            // for each of the page, make a clone
            for (u32 i = 0; i < s->npages; i++) {
                page_copy(dst, src, s->start + PAGE_SIZE * i, s->flags);
            }

            // add to the list of dst
            list_push_back(&dst->sections, &sec->node);
        }
    }
}
