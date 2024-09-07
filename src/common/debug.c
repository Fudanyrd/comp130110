#include "debug.h"
#include <common/defines.h>

typedef unsigned char uint8_t;
typedef unsigned long size_t;

void debug_init(void)
{
    // check some assumptions about the ISA.
    STATIC_ASSERT(sizeof(int) == 4);
    STATIC_ASSERT(sizeof(unsigned long) == 8);
    STATIC_ASSERT(sizeof(void *) == 8);

    // it is still highly recommended NOT to make any
    // assumptions about the ISA.

    // if there's other static members, initialize them.
}

/** Returns the next frame pointer. */
static inline void *next_fptr(void *fptr);

/** Returns the current program counter. */
static inline void *fptr2addr(void *fptr);

/** Print an unsigned byte. */
static inline void print_ub(uint8_t ub);

/** Print an unsigned long */
static inline void print_ul(size_t ul);

/** Print an unsigned char */
static void print_ch(uint8_t ch);

void backtrace(void)
{
    // the current frame pointer.
    void *fptr;
    // Arm ref manual page 34: x29 is frame pointer
    // asm volatile("ldr %0, [sp]" : "=r"(fptr));
    asm volatile("mov %0, x29" : "=r"(fptr));

    // clang-format off
    /**
     * calling stack:
     * +-------+
     * |  x30  |  
     * +-------+
     * |  x29  |
     * +-------+ <------------o
     * ...                    |
     * +-------+              |
     * |  x30  |              |
     * +-------+              |
     * |  x29  | -------------o
     * +-------+ 
     */
    // clang-format on

    printk("kernel backtrace: ");
    while (fptr != NULL) {
        printk("%p ", fptr2addr(fptr));
        fptr = next_fptr(fptr);
    }
    printk("\n");
}

void hexdump(uint8_t *start, size_t size, size_t off)
{
    static const char *sep = " | ";
    size_t b;

    while (size > 0UL) {
        b = size > 0x8 ? 0x8 : size;
        // print a base pointer.
        print_ul(off);

        // print hex number
        printk("%s", sep);
        for (size_t i = 0; i < b; i++) {
            print_ub(start[i]);
        }
        printk("| ");

        // print visible char and dots.
        printk("%s", sep);
        for (size_t i = 0; i < b; i++) {
            print_ch(start[i]);
        }
        printk("%s", sep);

        // clear;
        putch('\n');

        // advance
        size -= b;
        start += b;
        off += b;
    }

    return;
}

int Memchk(const uint8_t *start, size_t size, int val)
{
    for (size_t i = 0; i < size; i++) {
        if (start[i] != (uint8_t)(val & 0xff)) {
            return 0;
        }
    }
    return 1;
}

static inline void *next_fptr(void *fptr)
{
    return *(void **)(fptr);
}

static inline void *fptr2addr(void *fptr)
{
    void *ret = *(void **)(fptr + 0x8);
    return ret == NULL ? ret : ret - 0x4;
}

static inline void print_ub(uint8_t ub)
{
    const int hi = ((int)ub & 0xf0) >> 4;
    const int lo = ((int)ub & 0xf);

    if (hi > 9) {
        putch('a' + (hi - 0xa));
    } else {
        putch('0' + hi);
    }
    if (lo > 9) {
        putch('a' + (lo - 0xa));
    } else {
        putch('0' + lo);
    }
}

static inline void print_ul(size_t ul)
{
    putch('0');
    putch('x');
    static int buf[16];

    for (int i = 0; i < 16; i++) {
        buf[i] = (ul & 0xfL);
        ul = ul >> 4;
    }

    for (int i = 15; i >= 0; i--) {
        if (buf[i] > 0x9) {
            putch('a' + (buf[i] - 0xa));
        } else {
            putch('0' + buf[i]);
        }
    }

    putch(' ');
}

static void print_ch(uint8_t ch)
{
    static const char *spec = "~`!@#$%^&*()-_=+[]{}\\|:;\"\'<>,./";
    if (ch <= '9' && ch >= '0') {
        putch(ch);
        return;
    }
    if (ch <= 'Z' && ch >= 'A') {
        putch(ch);
        return;
    }
    if (ch <= 'z' && ch >= 'a') {
        putch(ch);
        return;
    }

    for (int i = 0; spec[i] != '\0'; i++) {
        if (spec[i] == ch) {
            putch(ch);
            return;
        }
    }

    putch('.');
}
