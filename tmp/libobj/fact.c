#include <stdint.h>

uint64_t fact(uint32_t n)
{
    uint64_t ret = 1;
    for (uint32_t i = 2; i <= n; i++) {
        ret *= i;
    }
    return ret;
}
