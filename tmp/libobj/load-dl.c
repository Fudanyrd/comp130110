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
    // printf("%ld\n", fn(3));

    dl_close(&sobj);

    // open another
    memset(&sobj, 0, sizeof(sobj));
    dl_open("intfn.so", &sobj);
    void *f;
    f = dl_load("foo", &sobj, &size);
    printf("%p,%ld\n", f, size);
    f = dl_load("bar", &sobj, &size);
    printf("%p,%ld\n", f, size);
    dl_close(&sobj);
    return 0;
}
