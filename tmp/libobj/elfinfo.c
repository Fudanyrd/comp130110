#include <assert.h>
#include <stdint.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define Round(foo, bar) ((uintptr_t)foo & ~(bar - 1))

struct elf_info {
    Elf64_Ehdr *eh;
    Elf64_Phdr *ph;
    Elf64_Shdr *sh;
    char *strtab;
    char *shstrtab;
    char *text;
    size_t stext;
    Elf64_Sym *symtab;
    size_t nsym;
};

#define HA ((void *)-1)

void elf_open(const char *fname, struct elf_info *ei)
{
    assert(ei != NULL);
    ei->text = NULL;
    int fd = open(fname, O_RDONLY);
    assert(fd >= 0);

    // elf header, prog header
    ei->eh = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(ei->eh != HA);
    ei->ph = (Elf64_Phdr *)((void *)ei->eh + ei->eh->e_phoff);

    // map loadable seg into memory
    for (Elf64_Half i = 0; i < ei->eh->e_phnum; i++) {
        Elf64_Phdr *p = ei->ph + i;
        // ref: elf.h:715
        switch (p->p_type) {
        case PT_LOAD: { // loadable segment
            int prot = 0; // page protectionn
            if (p->p_flags & PF_X) {
                // executable
                prot |= PROT_EXEC;
            }
            if (p->p_flags & PF_R) {
                // readable
                prot |= PROT_READ;
            }
            if (p->p_flags & PF_W) {
                // writable, do not load
                prot |= PROT_WRITE;
            }

            void *expected = ei->text + Round(p->p_vaddr, p->p_align);
            void *ret = mmap(expected,
                             p->p_memsz + (uintptr_t)p->p_vaddr % p->p_align,
                             prot,
                             MAP_PRIVATE | (expected == NULL ? 0 : MAP_FIXED),
                             fd, Round(p->p_offset, p->p_align));
            assert(ret != (void *)-1);

            if (p->p_flags == (PF_R | PF_X)) {
                ei->text = ret;
                ei->stext = p->p_memsz + (uintptr_t)p->p_vaddr % p->p_align;
            }
            break;
        }
        default:
            break;
        }
    }

    // read section headers
    Elf64_Shdr *sh;
    sh = malloc(sizeof(Elf64_Shdr) * ei->eh->e_shnum);
    assert(sh != (void *)0);
    assert(lseek(fd, ei->eh->e_shoff, SEEK_SET) != -1);
    read(fd, (void *)sh, sizeof(Elf64_Shdr) * ei->eh->e_shnum);
    ei->sh = sh;

    // content of .strtab
    char *strtab = NULL;
    // content of .shstrtab
    char *shstrtab = NULL;
    Elf64_Ehdr *eh = ei->eh;

    for (Elf64_Half i = 0; i < eh->e_shnum; i++) {
        // play with the section header
        // find all string tables: .strtab and .shstrtab
        switch (sh[i].sh_type) {
        case (SHT_STRTAB): {
            // hold a string table.
            size_t sz = sh[i].sh_size;
            void *buf = malloc(sz);
            assert(buf != NULL);
            assert(lseek(fd, sh[i].sh_offset, SEEK_SET));
            read(fd, buf, sz);
            if (i != eh->e_shstrndx) {
                strtab = buf;
            } else {
                shstrtab = buf;
            }
            break;
        }
        case (SHT_SYMTAB): {
            size_t sz = sh[i].sh_size;
            void *buf = malloc(sz);
            assert(buf != NULL);
            assert(lseek(fd, sh[i].sh_offset, SEEK_SET));
            read(fd, buf, sz);
            ei->symtab = buf;
            ei->nsym = sz / sizeof(Elf64_Sym);
            break;
        }
        default: {
            break;
        }
        }
    }

    ei->strtab = strtab;
    ei->shstrtab = shstrtab;
}

void *elf_sym(const char *name, struct elf_info *ei)
{
    void *ret = NULL;
    for (int i = 0; i < ei->nsym; i++) {
        Elf64_Sym *sym = &ei->symtab[i];
        if (strcmp(name, ei->strtab + sym->st_name) == 0) {
            ret = sym->st_value + ei->text;
            break;
        }
    }
    return ret;
}

void *elf_sec(const char *name, struct elf_info *ei, size_t *size)
{
    char *ret = NULL;
    for (Elf64_Half i = 0; i < ei->eh->e_shnum; i++) {
        Elf64_Shdr *s = &ei->sh[i];
        if (strcmp(name, ei->shstrtab + s->sh_name) == 0) {
            *size = (size_t)s->sh_size;
            ret = ei->text + s->sh_addr;
        }
    }

    return ret;
}

void elf_close(struct elf_info *ei)
{
    munmap(ei->eh, 4096);
    free(ei->sh);
    munmap(ei->text, ei->stext);
    free(ei->shstrtab);
    free(ei->strtab);
    free(ei->symtab);
    memset(ei, 0, sizeof(struct elf_info));
}

int main(int argc, char **argv)
{
    static struct elf_info ei;
    elf_open("intfn.so", &ei);
    printf("%p\n", ei.text);

    int (*foo)(int);
    int (*bar)(int);
    foo = elf_sym("foo", &ei);
    bar = elf_sym("bar", &ei);
    size_t gotsz;
    char *got = elf_sec(".got.plt", &ei, &gotsz);
    // relocate some symbols
    *(void **)(got + 0x20) = (void *)foo;
    *(void **)(got + 0x28) = (void *)bar;
    printf("%p %p %p\n", foo, bar, got);
    printf("0x%x, 0x%x\n", foo(0x12345678), bar(0x12345678));

    int (*baz)(int);
    baz = elf_sym("baz", &ei);
    printf("%p\n", baz);
    // use and then recover x16
    uint64_t x16;
    asm volatile("mov %0, x16" : "=r"(x16));
    printf("0x%x, 0x%x\n", baz(0x12345678), baz(0x12345678));
    asm volatile("mov x16, %0" : : "r"(x16));

    elf_close(&ei);

    return 0;
}
