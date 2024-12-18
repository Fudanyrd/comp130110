#include <kernel/mmap1217.h>
#include <fs/file1206.h>
#include <common/string.h>

#ifndef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif

static void *find_mmap_addr(struct pgdir *pd, u64 len);

u64 mmap(void *addr, u64 length, int prot, int flags, 
           int fd, isize offset)
{

    // check for parameters.
    if ((u64)addr & 0xfff || length & 0xfff || prot == 0) {
        // don't like the parameter. Abort
        return MMAP_FAILED;
    }

    Proc *proc = thisproc();
    File **ofile = (File **)proc->ofile.ofile;
    struct pgdir *pd = &proc->pgdir;

    if (flags & MAP_FIXED) {
        // nothing to be done.
    } else {
        addr = find_mmap_addr(pd, length);
    }

    if (addr == NULL || addr < (void *)MMAP_MIN_ADDR || 
        addr > (void *)MMAP_MAX_ADDR) {
        // do not map at addr.
        return MMAP_FAILED;
    }

    struct section *sec = kalloc(sizeof(struct section));
    sec->npages = length / PAGE_SIZE;
    sec->start = (u64)addr;
    sec->offset = 0;
    sec->fobj = NULL;

    // set flags
    sec->flags |= (prot & PROT_READ);
    sec->flags |= (prot & PROT_WRITE);
    sec->flags |= (prot & PROT_EXEC);

    File *fobj;
    if (fd >= 0 && fd < MAXOFILE && (fobj = ofile[fd]) != NULL) {
        if (fobj->type != FD_INODE) {
            // bad fd
            kfree(sec);
            return MMAP_FAILED;
        }
        Inode *ino = fobj->ino;
        ASSERT(ino->valid);
        if (ino->entry.type != INODE_REGULAR) {
            // bad fd
            kfree(sec);
            return MMAP_FAILED;
        }

        // create a file backend.
        sec->fobj = fshare(fobj);
        sec->offset = offset;
        sec->flags |= PF_F;
    }

    // do lazy page mapping.
    pgdir_add_section(pd, sec);
    return sec->start;
}

static void *find_mmap_addr(struct pgdir *pd, u64 len)
{
    struct list_elem *b; // before
    struct list_elem *a; // after
    struct section *sb; // section before
    struct section *sa; // section after
    struct list *lst = &pd->sections;

    b = list_begin(lst);
    // find the first section that has addr >= MMAP_MIN_ADDR.
    for (; b != list_end(lst); b = list_next(b)) {
        a = list_next(b);

        sb = list_entry(b, struct section, node);
        sa = list_entry(a, struct section, node);
        // the list is ordered.
        ASSERT(sb->start < sa->start);

        // left point of the unmapped area
        u64 l = MAX(sb->start + PAGE_SIZE * sb->npages, (u64)MMAP_MIN_ADDR);
        // right point of the unmapped area
        u64 r= MIN(sa->start, (u64)MMAP_MAX_ADDR);

        if (r > l && r - l > len * PAGE_SIZE) {
            return (void *)l;
        }
    }

    return NULL;
}

int section_unmap(struct pgdir *pd, struct section *sec)
{
    ASSERT(pd != NULL && sec != NULL);
    ASSERT((sec->start & 0xfff) == 0);

    for (u32 i = 0; i < sec->npages; i++) {

        // deallocate all pages.
        u64 addr = i * PAGE_SIZE + sec->start;
        PTEntry *pte = get_pte(pd, addr, false);

        // we accept pte to be NULL 
        // since mmap does lazy mmaping.
        if (pte != NULL && *pte != 0) {
            u64 pg = P2K(*pte & (~0xffful));
            kfree_page((void *)pg);
        }
    }

    if (sec->flags & PF_F) {
        ASSERT(sec->fobj != NULL);
        // FIXME: consider file write back
        // if writable.
        fclose(sec->fobj);
    }

    return 0;
}

struct section *section_search(struct pgdir *pd, u64 uva)
{
    if (uva == 0) {
        return NULL;
    }
    ASSERT(pd != NULL);

    struct list *lst = &pd->sections;
    struct list_elem *it = list_begin(lst);
    struct section *sec;

    for (; it != list_end(lst); it = list_next(it)) {
        sec = list_entry(it, struct section, node);
        if (sec->start <= uva && sec->start + sec->npages * PAGE_SIZE > uva) {
            return sec;
        }
    }

    return NULL;
}

int section_install(struct pgdir *pd, struct section *sec, u64 uva)
{
    ASSERT(pd != NULL && sec != NULL);
    PTEntry *pte = get_pte(pd, uva, true);
    if (pte == NULL) {
        return -1;
    }
    if (*pte != 0) {
        ASSERT(*pte & PTE_VALID);
        // this is caused by EACCESS, i.e.
        // write to read-only page
        return -1;
    }

    void *pg = kalloc_page();
    if (pg == NULL) {
        return -1;
    }

    // init page.
    if (sec->flags & PF_F) {
        // read from file.
        isize off = sec->offset + (uva - sec->start);
        Inode *ino = sec->fobj->ino;
        inodes.lock(ino);
        inodes.read(ino, pg, off, PAGE_SIZE);
        inodes.unlock(ino);
    } else {
        // zero-init
        memset(pg, 0, PAGE_SIZE);
    }

    pte = get_pte(pd, uva, true);
    *pte = K2P(pg);
    *pte |= PTE_USER_DATA;
    if ((sec->flags & PF_W) == 0) {
        // read-only
        *pte = *pte | PTE_RO;
    }
    return 0;
}
