#include "dl.h"
/**
 * An example showcasing the usage of dl library.
 */
#include <stdio.h>

static struct so sobj;
uint64_t (*fn)(uint32_t);

int main(int argc, char **argv)
{
    dl_open("fact.so", &sobj);
    size_t size;
    fn = dl_load("fact", &sobj, &size);
    printf("%p\n", fn);
    printf("%ld\n", size);
    printf("%ld\n", fn(3));

    dl_close(&sobj);
    return 0;
}
