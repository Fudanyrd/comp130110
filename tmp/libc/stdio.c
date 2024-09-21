#define syscall asm volatile("svc #0")
#include <syscall.h>
#include <stdint.h>
#include <stdarg.h>
#include "string.h"

static const char *digits = "0123456789abcdef";
static unsigned int putint(char *dst, int val);
static unsigned int putuint(char *dst, unsigned int val);
static unsigned int puthex(char *dst, unsigned int val);
static unsigned int putptr(char *dst, unsigned long val);
static unsigned int putl(char *dst, long val);
static unsigned int putstr(char *dst, const char *src);

void Puts(const char *s)
{
    asm volatile("mov x2, %0" : : "r"(Strlen(s)));
    asm volatile("mov x1, %0" : : "r"(s));
    asm volatile("mov x0, #1");
    asm volatile("mov x8, #64");
    syscall;
    return;
}

/**
 * Supported format: d(int), u(unsigned), x(for int32, unsigned32), 
 * p(for pointer, ulong), l(long), s(string).
 */
unsigned int Sprintf(char *dst, const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    unsigned ret = 0;

    for (int i = 0; fmt[i] != 0;) {
        if (fmt[i] == '%') {
            switch (fmt[i + 1]) {
            case 'd': {
                ret += putint(dst + ret, va_arg(arg, int));
                break;
            }
            case 'u': {
                ret += putuint(dst + ret, va_arg(arg, unsigned int));
                break;
            }
            case 'x': {
                ret += puthex(dst + ret, va_arg(arg, unsigned int));
                break;
            }
            case 'p': {
                ret += putptr(dst + ret, va_arg(arg, unsigned long));
                break;
            }
            case 'l': {
                ret += putl(dst + ret, va_arg(arg, long));
                break;
            }
            case 's': {
                ret += putstr(dst + ret, va_arg(arg, char *));
                break;
            }
            default: {
                dst[ret++] = '%';
                i--;
            }
            }
            i += 2;
        } else {
            dst[ret] = fmt[i];
            ret++;
            i++;
        }
    }
    dst[ret] = 0;

    va_end(arg);
    return ret;
}

static unsigned int putint(char *dst, int val)
{
    unsigned int ret = 0;
    char buf[16];
    unsigned int v;
    if (val < 0) {
        v = -val;
        dst[ret++] = '-';
    } else {
        // put negative sign
        v = val;
    }

    int i = 0;
    while (v > 0) {
        buf[i++] = digits[v % 0xa];
        v /= 0xa;
    }

    while (i > 0) {
        i--;
        dst[ret++] = buf[i];
    }
    return ret;
}

static unsigned int putuint(char *dst, unsigned int val)
{
    unsigned int ret = 0;
    char buf[16];
    int i = 0;
    while (val > 0) {
        buf[i++] = digits[val % 0xa];
        val /= 0xa;
    }

    while (i > 0) {
        i--;
        dst[ret++] = buf[i];
    }
    return ret;
}

static unsigned int puthex(char *dst, unsigned int val)
{
    unsigned int ret = 0;
    char buf[8];
    int i = 0;

    dst[ret++] = '0';
    dst[ret++] = 'x';
    while (val > 0) {
        buf[i++] = digits[val & 0xf];
        val = val >> 4;
    }

    while (i > 0) {
        i--;
        dst[ret++] = buf[i];
    }
    return ret;
}

static unsigned int putptr(char *dst, unsigned long val)
{
    unsigned int ret = 0;
    char buf[16];
    int i = 0;

    dst[ret++] = '0';
    dst[ret++] = 'x';
    while (val > 0) {
        buf[i++] = digits[val & 0xf];
        val = val >> 4ul;
    }

    while (i > 0) {
        i--;
        dst[ret++] = buf[i];
    }
    return ret;
}

static unsigned int putl(char *dst, long val)
{
    unsigned int ret = 0;
    unsigned long v;
    char buf[24];
    if (val < 0l) {
        dst[ret++] = '-';
        v = -val;
    } else {
        v = val;
    }

    int i = 0;
    while (v > 0) {
        buf[i++] = digits[v % 0xa];
        v /= 0xa;
    }
    while (i > 0) {
        i--;
        dst[ret++] = buf[i];
    }

    return ret;
}

static unsigned int putstr(char *dst, const char *src)
{
    unsigned int ret = 0;
    if (src == (const char *)0) {
        dst[ret++] = '(';
        dst[ret++] = 'n';
        dst[ret++] = 'u';
        dst[ret++] = 'l';
        dst[ret++] = 'l';
        dst[ret++] = ')';
        return ret;
    }

    while (*src != 0) {
        dst[ret++] = *src;
        src++;
    }
    return ret;
}

unsigned int Printf(const char *fmt, ...)
{
    static char buf[2048];
    char *dst = (char *)buf;
    va_list arg;
    va_start(arg, fmt);
    unsigned ret = 0;

    for (int i = 0; fmt[i] != 0;) {
        if (fmt[i] == '%') {
            switch (fmt[i + 1]) {
            case 'd': {
                ret += putint(dst + ret, va_arg(arg, int));
                break;
            }
            case 'u': {
                ret += putuint(dst + ret, va_arg(arg, unsigned int));
                break;
            }
            case 'x': {
                ret += puthex(dst + ret, va_arg(arg, unsigned int));
                break;
            }
            case 'p': {
                ret += putptr(dst + ret, va_arg(arg, unsigned long));
                break;
            }
            case 'l': {
                ret += putl(dst + ret, va_arg(arg, int64_t));
                break;
            }
            case 's': {
                ret += putstr(dst + ret, va_arg(arg, char *));
                break;
            }
            default: {
                dst[ret++] = '%';
                i--;
            }
            }
            i += 2;
        } else {
            dst[ret] = fmt[i];
            ret++;
            i++;
        }
    }
    dst[ret] = 0;
    va_end(arg);

    asm volatile("mov x2, %0" : : "r"(ret));
    asm volatile("mov x1, %0" : : "r"(dst));
    asm volatile("mov x0, #1");
    asm volatile("mov x8, #64");
    asm volatile("svc #0");
    return ret;
}
