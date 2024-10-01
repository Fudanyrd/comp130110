#include <stdio.h>
#include <stdlib.h>

void _start(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        Puts(argv[i]);
        Puts(" ");
    }
    Puts("\n");
    Exit(0);
}
