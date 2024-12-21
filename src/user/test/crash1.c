// check whether the kernel handles page fault.
#include "syscall.h"

int main(int argc, char **argv)
{
    sys_print("crash!", 6);
    int fd = sys_open("/init", O_RDONLY);
    sys_read(fd, (char *)main, 65536);
    sys_print("should be killed", 16);
    sys_close(fd);
    sys_exit(1);
}
