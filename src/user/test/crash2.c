// very similar to crash, crash1,
// but this time we try writing to
// mmap-read pages

#include "syscall.h"

int main(int argc, char **argv)
{
    sys_print("crash!", 6);
    int fd = sys_open("/init", O_RDONLY);
    if (fd < 0) {
        sys_print("Oh, open fail!", 15);
        sys_exit(10);
    }

    void *pt = sys_mmap(NULL, 65536, PROT_READ, MAP_PRIVATE, fd, 0);
    if (pt == MMAP_FAILED) {
        sys_print("Uh, mmap fail!", 15);
        sys_exit(10);
    }

    *(int *)pt = 0x1234fd;

    sys_print("should be killed", 16);
    sys_close(fd);
    sys_exit(1);
}
