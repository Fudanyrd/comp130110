// trick the kernel to do something silly
// should not panic the kernel

#include "syscall.h"

int main(int argc, char **argv)
{
    sys_write(2, "write to null.\n", 16);
    sys_write(1, NULL, 65536);
    sys_write(2, "write to null succeed??\n", 25);
    return 0;
}
