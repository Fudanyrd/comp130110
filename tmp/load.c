#include "sysutil.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage load <exe> argv\n");
    }
    loader(argv[1], argv + 1);
    return 0;
}
