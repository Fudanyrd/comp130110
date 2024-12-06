#include "exec1207.h"
#include <fs/file1206.h>
#include <kernel/mem.h>
#include <kernel/pt.h>

extern int exec(const char *path, char **argv)
{
    File *exe = fopen(path, F_READ);
    ASSERT(argv != NULL);

    Elf64_Ehdr *ehdr = kalloc(sizeof(Elf64_Ehdr));
    fread(exe, (char *)ehdr, sizeof(*ehdr));

    kfree(ehdr);
    fclose(exe);
    return 0;
}
