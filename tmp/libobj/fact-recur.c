#include <stdint.h>
uint64_t fact(uint32_t);

uint64_t fact(uint32_t n)
{
    if (n == 0) {
        return 1;
    }

    return fact(n - 1) * n;
}
