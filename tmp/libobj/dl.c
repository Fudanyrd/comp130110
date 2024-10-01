#include "dl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>

static void *mmap_fail(void)
{
    const char *error_message = dlerror();
    if (error_message) {
        fprintf(stderr, "mmap failed: %s\n", error_message);
    } else {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    }
    exit(EXIT_FAILURE);
}

static inline uintptr_t pg_round_up(uintptr_t size)
{
    return (size + 4095) & (~4095);
}

void dl_open(const char *fn, struct so *s)
{
    assert(s != NULL);
    int fd = open(fn, O_RDONLY);
    assert(fd >= 0);
    s->fd = fd;

    // read elf header
    Elf64_Ehdr *eh = &(s->eh);
    read(fd, (void *)eh, sizeof(Elf64_Ehdr));

    // read section headers
    Elf64_Shdr *sh;
    sh = malloc(sizeof(Elf64_Shdr) * eh->e_shnum);
    assert(sh != (void *)0);
    assert(lseek(fd, eh->e_shoff, SEEK_SET) != -1);
    read(fd, (void *)sh, sizeof(Elf64_Shdr) * eh->e_shnum);
    s->sh = sh;

    // content of .strtab
    char *strtab = NULL;
    // content of .shstrtab
    char *shstrtab = NULL;
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
        default: {
            break;
        }
        }
    }

    Elf64_Shdr *text = NULL;
    for (Elf64_Half i = 0; i < eh->e_shnum; i++) {
        if (strcmp(".text", shstrtab + sh[i].sh_name) == 0) {
            text = &sh[i];
            s->text_off = i;
            break;
        }
    }
    assert(text != NULL);

    s->shstrtab = shstrtab;
    s->strtab = strtab;

#ifndef MAP
    s->text = malloc(text->sh_size);
    assert(s->text != NULL);
    assert(lseek(fd, text->sh_offset, SEEK_SET) != -1);
    read(fd, s->text, text->sh_size);
#else
#define Round(foo, bar) ((uintptr_t)foo & ~(bar - 1))
    s->bias = text->sh_offset % 4096;
    s->text = mmap(NULL, text->sh_size + text->sh_offset % 4096,
                   PROT_READ | PROT_EXEC, MAP_PRIVATE, fd,
                   Round(text->sh_offset, 4096));
    if (s->text == (void *)-1) {
        mmap_fail();
    }
#endif
}

void *dl_load(const char *name, struct so *s, size_t *size)
{
    void *ret = NULL;

    // scan the symbol table for symbol "name"
    for (Elf64_Half i = 0; i < s->eh.e_shnum; i++) {
        switch (s->sh[i].sh_type) {
        case (SHT_SYMTAB): {
            size_t sz = s->sh[i].sh_size;
            void *buf = malloc(sz);
            assert(buf != NULL);
            assert(lseek(s->fd, s->sh[i].sh_offset, SEEK_SET));
            assert(read(s->fd, buf, sz) == sz);

            // read complete, read the symbol
            Elf64_Sym *sym = buf;
            for (size_t k = 0; k < sz / sizeof(Elf64_Sym); k++) {
                if (strcmp(name, s->strtab + sym[k].st_name) == 0) {
                    // got the symbol!
                    assert(sym[k].st_value >= s->sh[s->text_off].sh_addr);
                    ret = s->text + s->bias +
                          (sym[k].st_value - s->sh[s->text_off].sh_addr);
                    if (size != NULL) {
                        *size = (size_t)sym[k].st_size;
                    }
                    free(buf);
                    return ret;
                }
            }
            free(buf);
            break;
        }
        }
    }

    return ret;
}

void dl_close(struct so *s)
{
    close(s->fd);
    free(s->strtab);
    free(s->shstrtab);
    free(s->sh);
#ifndef MAP
    free(s->text);
#else
    munmap(s->text, s->sh[s->text_off].sh_size + s->bias);
#endif
}
