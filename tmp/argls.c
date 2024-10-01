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

#include <stdio.h>
#include <stdlib.h>

void main(int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        Puts(argv[i]);
        Puts("\n");
    }
    Exit(0);
}
