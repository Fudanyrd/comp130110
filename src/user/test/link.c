#include "syscall.h"

int link(const char *oldpath, const char *newpath);

int main(int argc, char **argv)
{
    if (argc < 3) {
        sys_write(2, "Usage: link old new\n", 21);
        return 1;
    }

    if (link(argv[1], argv[2]) != 0) {
        sys_write(2, "cannot link\n", 13);
        return 1;
    }

    // success
    return 0;
}

asm (
    ".globl link\n\t"
    "link:\n\t"
    "mov w8, #21\n\t"
    "svc #0\n\t"
    "ret\n"
);
