#define syscall asm volatile("svc #0")
#include <syscall.h>
#include <stdint.h>
#include "string.h"

void Puts(const char *s)
{
    asm volatile("mov x2, %0" : : "r"(Strlen(s)));
    asm volatile("mov x1, %0" : : "r"(s));
    asm volatile("mov x0, #1");
    asm volatile("mov x8, #64");
    syscall;
    return;
}
