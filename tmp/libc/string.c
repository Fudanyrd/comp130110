#include <stdint.h>

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
