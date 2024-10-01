#include <stdio.h>
#include "sysutil.h"
#include <string.h>
#include <unistd.h>

/**
 * Fudanyrd's tiny shell: ysh
 */
int main(int argc, char **argv)
{
    static char line[8192];
    const char *pmt = "ysh> ";
    write(1, pmt, strlen(pmt));
    while (fgets(line, sizeof(line), stdin) != NULL) {
        for (int i = 0; line[i] != 0; i++) {
            if (line[i] == '\r' || line[i] == '\n') {
                line[i] = ' ';
            }
        }
        System(line);
        write(1, pmt, strlen(pmt));
    }
    return 0;
}
