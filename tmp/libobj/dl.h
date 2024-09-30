#ifndef __DL_H_
#define __DL_H_

#include <elf.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// how to allocate memory for .text
#define MAP
#undef MAP

// a shared object
struct so {
    // elf header
    Elf64_Ehdr eh;
    // section header 
    Elf64_Shdr *sh;
    // string table
    char *strtab;
    // section string table
    char *shstrtab;
    // mapped text(code) section
    void *text;
    // text section offset
    int text_off;
    // file descriptor
    int fd;
};

void dl_open(const char *fn, struct so *s);
void *dl_load(const char *name, struct so *s, size_t *size);
void dl_close(struct so *s);

#endif // __DL_H_
