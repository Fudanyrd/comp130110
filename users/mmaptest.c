// mmap test from previous year.

#include "syscall.h"
#ifdef MMAP_MIN_ADDR
#undef MMAP_MIN_ADDR
#endif

#define MMAP_MIN_ADDR ((void *)0x60000000)
#define PAGE_SIZE 4096
#define open sys_open
#define exit sys_exit
#define mmap sys_mmap
#define munmap sys_munmap
#define read sys_read
#define write sys_write
#define close sys_close
static char buf[6144];

static char digits[] = "0123456789ABCDEF";

static int strcmp(const char *a, const char *b)
{
    int i = 0;
    for (; a[i] != 0 && b[i] != 0; i++) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }

    return (int)a[i] - (int)b[i];
}

static void deletefile(const char *name);

static void printint(int fd, int xx, int base, int sgn)
{
    char buf[16];
    int i, neg;
    u32 x;

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
        write(fd, &buf[i], 1);
}

static void mmaptest();
static void forktest();
static void collidetest();

int main(int argc, char **argv)
{
    mmaptest();
    forktest();
    collidetest();
    sys_print("All tests passed.", 18);
    sys_print("cleaning...", 12);

    // remove the tmp file
    const char *fn = "mmap.dur";
    deletefile(fn);
    sys_exit(0);
}

void _v1(char *p)
{
    for (int i = 0; i < PAGE_SIZE * 2; i++) {
        if (i < PAGE_SIZE + (PAGE_SIZE / 2)) {
            if (p[i] != 'A') {
                // fail
                sys_write(2, "mismatch at ", 13);
                printint(2, i, 10, 0);
                sys_write(2, "\n", 2);
            }
        } else {
            if (p[i] != 0) {
                sys_write(2, "mismatch at ", 13);
                printint(2, i, 10, 0);
                sys_write(2, "\n", 2);
            }
        }
    }
}

void makefile(const char *name)
{
    int fd = sys_open(name, O_TRUNC | O_WRITE | O_CREATE);
    if (fd < 0) {
        sys_write(2, "cannot open\n", 13);
        sys_exit(1);
    }

    for (usize i = 0; i < sizeof(buf); i++) {
        buf[i] = 'A';
    }
    if (sys_write(fd, buf, sizeof(buf)) != sizeof(buf)) {
        sys_write(2, "makefile: write\n", 17);
        sys_exit(1);
    }

    sys_close(fd);
}

static void mmaptest()
{
    const char *f = "mmap.dur";
    makefile(f);

    int fd;
    char *p;

    write(1, "Test open\n", 10);
    fd = sys_open(f, O_READ);
    if (fd < 0) {
        sys_write(1, "mmap test open\n", 16);
        exit(1);
    }
    p = mmap(NULL, PAGE_SIZE * 2, PROT_READ, MAP_PRIVATE, fd, 0);
    _v1(p);
    if (munmap(p, PAGE_SIZE * 2) != 0) {
        write(2, "mmaptest: unmap\n", 17);
        exit(1);
    }
    write(1, "Open PASS\n", 10);

    write(1, "Test Invalid\n", 14);
    p = mmap(NULL, PAGE_SIZE * 3, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p != MMAP_FAILED) {
        write(2, "mmaptest: should fail\n", 23);
        exit(1);
    }
    sys_close(fd);
    write(1, "Invalid PASS\n", 14);

    // test map dirty.
    write(1, "Test Dirty\n", 12);
    fd = open(f, O_READ | O_WRITE);
    if (fd < 0) {
        write(2, "mmaptest: open\n", 16);
    }

    p = mmap(NULL, PAGE_SIZE * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    for (int i = 0; i < PAGE_SIZE * 2; i++) {
        p[i] = 'Z';
    }
    if (munmap(p, PAGE_SIZE * 2) != 0) {
        write(2, "mmaptest: unmap\n", 17);
        exit(1);
    }
    close(fd);
    write(1, "Dirty unmap\n", 13);

    // check that the file is changed.
    fd = open(f, O_READ | O_WRITE);
    for (int i = 0; i < sizeof(buf); i++) {
        char b;
        if (read(fd, &b, 1) != 1) {
            write(2, "mmaptest: read\n", 16);
            exit(1);
        }
        if (b != 'Z') {
            write(2, "mmaptest: persist\n", 19);
            exit(1);
        }
    }
    close(fd);
    write(1, "Dirty PASS\n", 12);

    // test two mmap
    write(1, "Test map 2\n", 12);
    int fd1 = open("mmap1", O_CREATE | O_READ | O_WRITE);
    if (write(fd1, "12345", 5) != 5) {
        write(2, "mmaptest: write fd1\n", 21);
        exit(1);
    }
    int fd2 = open("mmap2", O_CREATE | O_READ | O_WRITE);
    if (write(fd2, "56789", 5) != 5) {
        write(2, "mmaptest: write fd2\n", 21);
        exit(1);
    }

    void *p1 =
            mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd1, 0);
    void *p2 =
            mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd2, 0);
    if (strcmp("12345", p1) != 0) {
        write(2, "mmaptest: map fd1\n", 19);
        exit(1);
    }
    if (strcmp("56789", p2) != 0) {
        write(2, "mmaptest: map fd2\n", 19);
        exit(1);
    }
    close(fd1);
    close(fd2);
    deletefile("mmap1");
    deletefile("mmap2");
    munmap(p1, PAGE_SIZE);
    munmap(p2, PAGE_SIZE);
    write(1, "Map 2 PASS\n", 12);
}

static void forktest()
{
    const char *f = "mmap.dur";
    makefile(f);

    write(1, "Test fork\n", 11);
    int fd = open(f, O_RDONLY);
    char *p1 = mmap(NULL, PAGE_SIZE * 2, PROT_READ, MAP_SHARED, fd, 0);
    if (p1 == MMAP_FAILED) {
        write(2, "mmap fail\n", 10);
        exit(1);
    }
    char *p2 = mmap(NULL, PAGE_SIZE * 2, PROT_READ, MAP_SHARED, fd, 0);
    if (p2 == MMAP_FAILED) {
        write(2, "mmap fail\n", 10);
        exit(1);
    }

    int id = sys_fork();
    if (id < 0) {
        write(2, "fork\n", 5);
        exit(1);
    }
    if (id == 0) {
        _v1(p1);
        if (munmap(p1, PAGE_SIZE) != 0) {
            write(2, "child unmap fail\n", 18);
            exit(1);
        }
        write(1, "Child OK\n", 10);
        exit(0);
    }

    sys_wait(&id);
    _v1(p1);
    _v1(p2);
    munmap(p1, PAGE_SIZE * 2);
    munmap(p2, PAGE_SIZE * 2);
    write(1, "Fork PASS\n", 11);
}

// If a file exists, unlink it.
static void deletefile(const char *name)
{
    int fd = open(name, O_READ);
    if (fd >= 0) {
        close(fd);
        if (sys_unlink(name) != 0) {
            sys_write(2, "cannnot unlink, abort\n", 23);
            exit(1);
        }
    }
}

// [New] check MAP_FIXED and collision detect
static void collidetest()
{
    write(1, "Test collison\n", 15);

    // get a initial address
    void *start = sys_mmap((void *)MMAP_MIN_ADDR, PAGE_SIZE,
                           PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, -1,
                           0);

    if (start != MMAP_MIN_ADDR) {
        write(2, "start: mmap failed\n", 20);
        exit(1);
    }

    // mmap another 2, which should succeed.
    void *p1 = sys_mmap(start + PAGE_SIZE, PAGE_SIZE * 2, PROT_READ,
                        MAP_FIXED | MAP_PRIVATE, -1, 0);
    void *p2 = sys_mmap(start + PAGE_SIZE * 3, PAGE_SIZE * 3, PROT_READ,
                        MAP_FIXED | MAP_PRIVATE, -1, 0);

    // clang-format off
    // current mapping:
    // [ ]     [ ]  [ ] [ ] [ ] [ ]
    // ^start  ^ p1     ^p2
    // clang-format on

    if (p1 != start + PAGE_SIZE) {
        write(2, "p1: mmap failed\n", 17);
        exit(1);
    }
    if (p2 != start + PAGE_SIZE * 3) {
        write(2, "p2: mmap failed\n", 17);
        exit(1);
    }

    // try mapping in the area
    void *pt =
            mmap(start + PAGE_SIZE, PAGE_SIZE * 4, PROT_READ, MAP_FIXED, -1, 0);
    if (pt != MMAP_FAILED) {
        write(2, "pt: mmap should fail\n", 22);
        exit(1);
    }

    // munamp p1. current scenario:
    munmap(p1, PAGE_SIZE * 2);
    // clang-format off
    // current mapping:
    // [ ]     [I]  [I] [ ] [ ] [ ]
    // ^start           ^p2
    // clang-format on

    // collide start.
    pt = mmap(start + PAGE_SIZE * 3, PAGE_SIZE, PROT_READ, MAP_FIXED, -1, 0);
    if (pt != MMAP_FAILED) {
        write(2, "pt: mmap should fail\n", 22);
        exit(1);
    }

    // collide end.
    pt = mmap(start + PAGE_SIZE * 2, PAGE_SIZE * 2, PROT_READ, MAP_FIXED, -1,
              0);
    if (pt != MMAP_FAILED) {
        write(2, "pt: mmap should fail\n", 22);
        exit(1);
    }

    // remap p1.
    p1 = mmap(start + PAGE_SIZE, PAGE_SIZE * 2, PROT_READ, MAP_FIXED, -1, 0);
    if (p1 != start + PAGE_SIZE) {
        write(2, "remap p1 failed\n", 17);
        exit(1);
    }

    // done. clean up.
    munmap(start, PAGE_SIZE);
    munmap(p1, PAGE_SIZE * 2);
    munmap(p2, PAGE_SIZE * 3);

    write(1, "collison PASS\n", 15);
}
