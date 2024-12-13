#pragma once

#include <aarch64/mmu.h>
#include <fdutil/lst.h>

struct section {
    u64 start; // start address
    struct list_elem node;
    u32 npages; // number of pages
    u32 flags; // flags(r,w,x)
};

struct pgdir {
    PTEntriesPtr pt;
    // sections, ascending order wrt. start vaddr
    struct list sections;
};

void init_pgdir(struct pgdir *pgdir);
WARN_RESULT PTEntriesPtr get_pte(struct pgdir *pgdir, u64 va, bool alloc);
void free_pgdir(struct pgdir *pgdir);
void attach_pgdir(struct pgdir *pgdir);

/** Add a section to pgdir. 
 * @param sec a section element allocated by kalloc().
 */
void pgdir_add_section(struct pgdir *pgdir, struct section *sec);

/** Create a clone of pgdir. Used by fork(). 
 * @param dst a initialized page dir.
 * @param src the pgdir from which to make a copy
 */
void pgdir_clone(struct pgdir *dst, struct pgdir *src);
