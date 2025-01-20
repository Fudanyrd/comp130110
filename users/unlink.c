#include "syscall.h"

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (sys_unlink(argv[i]) < 0) {
            sys_write(1, "unlink FAIL\n", 12);
        }
    }
    return 0;
}
