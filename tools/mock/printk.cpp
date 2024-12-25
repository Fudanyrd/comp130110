extern "C" {

// make printk() arch-independent.
#include <stdio.h>
#include <stdarg.h>

void printk(const char *fmt, ...)
{
    va_list lst;
    va_start(lst, fmt);
    vprintf(fmt, lst);
}

}