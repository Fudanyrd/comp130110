#pragma once
#include <kernel/pt.h>
#include <kernel/exec1207.h>

#define MMAP_MIN_ADDR 0x60000000
#define MMAP_MAX_ADDR 0xb0000000

// return value
#define MMAP_FAILED ((u64)-1)

// prots

#define PROT_READ PF_R
#define PROT_WRITE PF_W
#define PROT_EXEC PF_X

// flags

#define MAP_PRIVATE 1
#define MAP_FIXED 2

// mmap syscall executor
u64 mmap(void *addr, u64 length, int prot, int flags, 
           int fd, isize offset);

int munmap(void *addr, u64 length);

// kernel helper methods. 

/** Destroy page mapping according to sec. Will NOT free sec.
 * @return 0 if successful.
 */
extern int section_unmap(struct pgdir *pd, struct section *sec);

// return the section that guards uva.
// null if not found. 
extern struct section *section_search(struct pgdir *pd, u64 uva);

// fetch missing page at uva.
// return 0 if the page is installed at uva.
extern int section_install(struct pgdir *pd, struct section *sec, u64 uva);
