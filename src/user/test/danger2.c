// we mmap 3 pages, and only unmap the page
// in the middle.
// The program should crash.

#include "syscall.h"

int main(int argc, char **argv)
{
    void *addr = sys_mmap(NULL, 4096 * 3, PROT_READ | PROT_WRITE, MAP_PRIVATE,
                          -1, 0);
    if (addr == MMAP_FAILED) {
        sys_print("mmap FAIL", 9);
        return 1;
    }

    if (sys_munmap(addr + 4096, 4096) != 0) {
        sys_print("munmap FAIL", 11);
        return 1;
    }

    // try writing to first page.
    *(int *)addr = 0x1234fd;
    // try the third?
    *(int *)(addr + 4096 * 2) = 0xfd1234;

    // crash!
    sys_print("crash!", 6);
    *(int *)(addr + 4096 * 1) = 0xfd1234;
    sys_print("should crash", 13);
    return 0;
}
