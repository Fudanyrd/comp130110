// check whether the kernel handles page fault.
#include "syscall.h"

int main(int argc, char **argv) 
{
    sys_print("crash!", 6);
    *(int *)0x60000000 = 4;
    sys_print("should be killed", 16);
    sys_exit(1);
}
