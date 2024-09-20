#include <syscall.h>
#include <stdint.h>

/** argls(argument list)
 * This file only works in freestanding environment,
 * and must be statically linked.
 * 
 * To compile and run the program:
 * gcc -S argls.c && gcc -static (-g) -c argls && ld argls.o 
 */

#define syscall asm volatile("svc #0")
#define main _start

static uintptr_t Strlen(const char *s)
{
    if (s == (void *)0) {
        return 0;
    }

    uintptr_t i = 0;
    while (s[i] != 0) {
        i++;
    }
    return i;
}

static void Puts(const char *s)
{
    asm volatile("mov x2, %0" : : "r"(Strlen(s)));
    asm volatile("mov x1, %0" : : "r"(s));
    asm volatile("mov x0, #1");
    asm volatile("mov w8, #64");
    syscall;
    return;
}

static void Exit(int code)
{
    asm volatile("mov w8, #93");
    asm volatile("mov x0, %0" : : "r"(code));
    syscall;
}

void main(int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        Puts(argv[i]);
        Puts("\n");
    }
    Exit(0);
}
