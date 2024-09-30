#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <assert.h>

void dumphex(unsigned int val)
{
    if (val < 0xa) {
        putchar('0' + val);
    } else {
        putchar('a' + val);
    }
}

void dumpb(uint8_t *buf, size_t st, size_t sz)
{
    size_t cnt = 0;
    for (size_t i = st; i < st + sz; i++) {
        dumphex((buf[i] & 0xf0) >> 4);
        dumphex(buf[i] & 0xf);

        cnt++;
        if (cnt % 4 == 0) {
            putchar('\n');
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: obj <elf>\n");
        exit(0);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("failure");
        exit(0);
    }

    // elf header
    static Elf64_Ehdr eh;
    read(fd, (void *)&eh, sizeof(eh));

    // section header table
    Elf64_Shdr *sh;
    sh = malloc(sizeof(Elf64_Shdr) * eh.e_shnum);
    assert(sh != (void *)0);
    assert(lseek(fd, eh.e_shoff, SEEK_SET) != -1);
    read(fd, (void *)sh, sizeof(Elf64_Shdr) * eh.e_shnum);

    // content of .strtab
    char *strtab = NULL;
    // content of .shstrtab
    char *shstrtab = NULL;
    for (Elf64_Half i = 0; i < eh.e_shnum; i++) {
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
            if (i != eh.e_shstrndx) {
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

    for (Elf64_Half i = 0; i < eh.e_shnum; i++) {
        // play with the section header
        switch (sh[i].sh_type) {
        case (SHT_SYMTAB): {
            // hold a symbol table.
            size_t sz = sh[i].sh_size;
            void *buf = malloc(sz);
            assert(buf != NULL);
            assert(lseek(fd, sh[i].sh_offset, SEEK_SET));
            read(fd, buf, sz);
            // dump to stdout
            Elf64_Sym *sym = buf;

            for (size_t k = 0; k < sz / sizeof(Elf64_Sym); k++) {
                if (sym[k].st_info == STT_FUNC) {
                    printf("%s,%p,function\n", strtab + sym[k].st_name,
                           (void *)sym[k].st_value);
                } else {
                    printf("%s,%p,unknown\n", strtab + sym[k].st_name,
                           (void *)sym[k].st_value);
                }
            }
            free(buf);
            break;
        }
        default: {
            break;
        }
        }
    }

    free(strtab);
    free(shstrtab);
    free(sh);
    close(fd);
    return 0;
}
