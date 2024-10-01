#include "dl.h"
/**
 * An example showcasing the usage of dl library.
 */
#include <stdio.h>

static struct so sobj;
uint64_t (*fn)(uint32_t);

int main(int argc, char **argv)
{
    dl_open("fact-recur.so", &sobj);
    size_t size;
    fn = dl_load("fact", &sobj, &size);
    printf("%p\n", fn);
    printf("%ld\n", size);

    for (uint32_t i = 0; i < 6; i++) {
        printf("%ld\n", fn(i));
    }

    dl_close(&sobj);

    // open another
    memset(&sobj, 0, sizeof(sobj));
    dl_open("intfn.so", &sobj);
    int (*foo)(int);
    int (*bar)(int);
    foo = dl_load("foo", &sobj, NULL);
    bar = dl_load("bar", &sobj, NULL);
    printf("0x%x 0x%x\n", foo(0x4080), bar(0x4080));
    dl_close(&sobj);

    // open another
#if 0
    memset(&sobj, 0, sizeof(sobj));
    dl_open("intfn.so", &sobj);
    void *f;
    f = dl_load("foo", &sobj, &size);
    printf("%p,%ld\n", f, size);
    f = dl_load("bar", &sobj, &size);
    printf("%p,%ld\n", f, size);
    dl_close(&sobj);
#endif
    return 0;
}
