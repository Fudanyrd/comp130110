#include "exec1207.h"
#include <fs/file1206.h>
#include <kernel/mem.h>
#include <kernel/pt.h>
#include <common/string.h>

// see aarch64/trap.S.
extern void fork_return(u64 sp, u64 elr, u64 spsr);

/** Install the section into page table.
 * @return 0 on success. 
 */
static int install_section(struct pgdir *pd, Elf64_Phdr *ph, File *exe);

extern int exec(const char *path, char **argv)
{
    File *exe = fopen(path, F_READ);
    ASSERT(argv != NULL);
    Proc *proc = thisproc();
    struct pgdir *pd = &proc->pgdir;

    Elf64_Ehdr *ehdr = kalloc(sizeof(Elf64_Ehdr));
    if (ehdr == NULL) {
        // must fail
        goto exec_bad;
    }
    fread(exe, (char *)ehdr, sizeof(*ehdr));

    Elf64_Phdr *phdr = kalloc(sizeof(Elf64_Phdr));
    if (phdr == NULL) {
        // must fail
        kfree(ehdr);
        goto exec_bad;
    }

    // read each segment from disk.
    for (Elf64_Half i = 0; i < ehdr->e_phnum; i++) {
        isize offset = ehdr->e_phoff + i * sizeof(Elf64_Phdr);
        ASSERT(fseek(exe, offset, S_SET) == offset);
        // read the program header.
        fread(exe, (char *)phdr, sizeof(Elf64_Phdr));

        if (phdr->p_type == PT_LOAD) {
            // load into page table.
            if (install_section(pd, phdr, exe) != 0) {
                goto exec_bad;
            }
        }
    }

    // map the space for stack.
    PTEntry *entr = get_pte(pd, STACK_START - PAGE_SIZE, true);
    void *stkpg = kalloc_page();
    *entr = K2P(stkpg) | PTE_USER_DATA;

    // setup a user context for trap_return.
    UserContext *ctx = proc->ucontext;
    memset(ctx, 0, sizeof(*ctx));
    ctx->elr = ehdr->e_entry;
    // see aarch64/trap.S for why set x0 to this val.
    // ctx->spsr = 0;

    // FIXME: set up argc, argv in user stack
    char *sp = (char *)stkpg + PAGE_SIZE;
    u64 cur = STACK_START; // cur advances in the same speed as sp.
    u64 addrs[32];
    int i;
    for (i = 0; argv[i] != NULL; i++) {
        // put these strings on stack.
        usize len = strlen(argv[i]);
        sp -= (len + 1);
        cur -= (len + 1);
        addrs[i] = cur;
        // strcpy
        for (usize k = 0; k < len; k++) {
            sp[k] = argv[i][k];
        }
        sp[len] = 0;
    }
    const u64 narg = (u64)i;
    // align to 16 byte for storing pointers.
    sp -= (u64)sp % 0x10;
    if (narg % 2 != 0) {
        sp -= 0x8;
    }
    // push NULL.
    sp -= 0x8;
    *(char **)sp = NULL;
    // push argv, argc. 
    for (i --; i >= 0; i--) {
        sp -= 0x8;
        *(u64 *)sp = addrs[i];
    }
    sp -= 8;
    *(u64 *)sp = narg;
    // must align to 16 bytes.
    ASSERT((u64) sp % 0x10 == 0);
    // set x0, sp to the current pointer.
    ctx->sp = ctx->x0 = STACK_START - PAGE_SIZE + (sp - (char *)stkpg);

    // free allocated resources.
    kfree(ehdr);
    kfree(phdr);
    fclose(exe);

    // install the page table.
    attach_pgdir(pd);

    // set ctx so as to jump here.
    proc->ucontext = ctx;
    return 0;

exec_bad:
    return -1;
}

static inline void *addr_round_down(u64 addr, u64 align) 
{
    return (void *)(addr - addr % align);
}
static inline void *addr_round_up(u64 addr, u64 align)
{
    return addr_round_down(addr + align - 1, align);
}

static int install_section(struct pgdir *pd, Elf64_Phdr *ph, File *exe)
{

    // use jyy's static loader as a reference.
    // url: https://jyywiki.cn/pages/OS/2022/demos/loader-static.c
    // pay attention to page initialization and alignment.

    // protection
    PTEntry prot = PTE_USER_DATA;
    void *start = addr_round_down(ph->p_vaddr, PAGE_SIZE); 
    void *end = addr_round_up(ph->p_vaddr + ph->p_memsz, PAGE_SIZE);
    isize offset = ph->p_offset;
    isize nread = ph->p_filesz;

    // load the first page, must read from file.
    if (start < end && nread > 0) {
        PTEntry *entr = get_pte(pd, (u64)start, true);
        if (entr == NULL) {
            goto install_bad;
        }

        // set the page to 0.
        void *pg = kalloc_page();
        memset(pg, 0, PAGE_SIZE);

        // read from file
        ASSERT(fseek(exe, offset, S_SET) == offset);
        // tmp = min(4096 - vaddr % 4096, nread);
        isize tmp = PAGE_SIZE - ph->p_vaddr % PAGE_SIZE;
        tmp = tmp > nread ? nread : tmp;
        fread(exe, (char *)pg + ph->p_vaddr % PAGE_SIZE, tmp);

        // install the page
        *entr = K2P(pg) | PTE_USER_DATA;

        // advance.
        start += PAGE_SIZE;
        offset += tmp;
        nread -= tmp;
    }

    for (; start < end; start += PAGE_SIZE) {
        // map the page
        PTEntry *entr = get_pte(pd, (u64)start, true);
        if (entr == NULL) {
            goto install_bad;
        }

        void *pg = kalloc_page();
        // load from file.
        ASSERT(fseek(exe, offset, S_SET) == offset);
        if (nread >= PAGE_SIZE) {
            // read an entire page.
            fread(exe, pg, PAGE_SIZE);
            nread -= PAGE_SIZE;
        } else {
            if (nread != 0)
                fread(exe, pg, nread);
            // init the rest with zero.
            memset(pg + nread, 0, PAGE_SIZE - nread);
            nread = 0;
        }
        *entr = K2P(pg) | prot;

        // advance.
        offset += PAGE_SIZE;
    }

    return 0;
install_bad:
    return -1;
}