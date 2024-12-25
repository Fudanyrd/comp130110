// trick the kernel to do something silly
// should not panic the kernel

#include "syscall.h"

int main(int argc, char **argv)
{
    int fd = sys_open("/bin/sh", O_READ);
    if (fd < 0) {
        sys_write(2, "open fail\n", 10);
        sys_exit(1);
    }
    sys_write(2, "read to null\n", 13);
    sys_read(fd, NULL, 4096);
    sys_write(2, "read to null succeed??\n", 24);
    return 0;
}
