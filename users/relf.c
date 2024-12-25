// a lightweight readelf.

#include "syscall.h"

// ulib.c
typedef u8 uchar;
typedef u64 uint64;
typedef u32 uint;

#include <stdarg.h>
#define write sys_write

static char digits[] = "0123456789ABCDEF";

static void putc(int fd, char c)
{
    write(fd, &c, 1);
}

static void printint(int fd, int xx, int base, int sgn)
{
    char buf[16];
    int i, neg;
    uint x;

    neg = 0;
    if (sgn && xx < 0) {
        neg = 1;
        x = -xx;
    } else {
        x = xx;
    }

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);
    if (neg)
        buf[i++] = '-';

    while (--i >= 0)
        putc(fd, buf[i]);
}

static void printptr(int fd, uint64 x)
{
    int i;
    putc(fd, '0');
    putc(fd, 'x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        putc(fd, digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void vprintf(int fd, const char *fmt, va_list ap)
{
    char *s;
    int c, i, state;

    state = 0;
    for (i = 0; fmt[i]; i++) {
        c = fmt[i] & 0xff;
        if (state == 0) {
            if (c == '%') {
                state = '%';
            } else {
                putc(fd, c);
            }
        } else if (state == '%') {
            if (c == 'd') {
                printint(fd, va_arg(ap, int), 10, 1);
            } else if (c == 'l') {
                printint(fd, va_arg(ap, uint64), 10, 0);
            } else if (c == 'x') {
                printint(fd, va_arg(ap, int), 16, 0);
            } else if (c == 'p') {
                printptr(fd, va_arg(ap, uint64));
            } else if (c == 's') {
                s = va_arg(ap, char *);
                if (s == 0)
                    s = "(null)";
                while (*s != 0) {
                    putc(fd, *s);
                    s++;
                }
            } else if (c == 'c') {
                putc(fd, va_arg(ap, uint));
            } else if (c == '%') {
                putc(fd, c);
            } else {
                // Unknown % sequence.  Print it to draw attention.
                putc(fd, '%');
                putc(fd, c);
            }
            state = 0;
        }
    }
}

void fprintf(int fd, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fd, fmt, ap);
}

void printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(1, fmt, ap);
}
/* Return the largest c that c is a multiple of b and c <= a. */
static inline u64 round_down(u64 a, u64 b)
{
    return a - a % b;
}

/* Return the smallest c that c is a multiple of b and c >= a. */
static inline u64 round_up(u64 a, u64 b)
{
    return round_down(a + b - 1, b);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        sys_write(2, "Usage: relf file\n", 18);
        return 1;
    }

    int fd = sys_open(argv[1], O_READ);
    if (fd < 0) {
        sys_write(2, "cannot open\n", 13);
        return 1;
    }

    static InodeEntry entry;
    if (sys_fstat(fd, &entry) < 0) {
        sys_write(2, "cannot stat\n", 13);
        return 1;
    }

    u64 len = round_up(entry.num_bytes, 4096);
    void *map = sys_mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MMAP_FAILED) {
        sys_write(2, "cannot mmap\n", 13);
        return 1;
    }

    Elf64_Ehdr *ehdr = map;
    sys_write(1, "Magic: ", 8);
    // tricky: ehdr is not loaded.
    // but we ask the kernel to read from this addr.
    sys_write(1, (char *)ehdr->e_ident, 16);
    sys_write(1, "\n", 1);
    fprintf(1, "Program header: %d\n", ehdr->e_phnum);
    fprintf(1, "Section header: %d\n", ehdr->e_shnum);

    // read program header
    Elf64_Phdr *phdr = (Elf64_Phdr *)(map + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        fprintf(1, "[%d] Program Header: \n", i);
        fprintf(1, "  Start Address: %p, Size: 0x%x\n", (void *)phdr[i].p_vaddr,
                phdr[i].p_memsz);
    }

    // clear the mmaped area.
    if (sys_munmap(map, len) != 0) {
        sys_write(2, "munmap fail\n", 13);
        return 1;
    }

    return 0;
}
