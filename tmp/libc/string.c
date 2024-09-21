#include <stdint.h>
#include <stddef.h>

uintptr_t Strlen(const char *s)
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

void *memset(uint8_t *b, int v, size_t sz)
{
    for (size_t i = 0; i < sz; i++) {
        b[i] = (uint8_t)(v & 0xff);
    }
}
