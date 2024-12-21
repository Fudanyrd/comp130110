// low-profile head:
// read the first several bytes from stdin
#include "syscall.h"

static long atoi(const char *s)
{
    long n;

    n = 0;
    while ('0' <= *s && *s <= '9')
        n = n * 10 + *s++ - '0';
    return n;
}

int main(int argc, char **argv)
{
    long nb = 0;
    if (argc > 1) {
        nb = atoi(argv[1]);
    }

    static char buf[512];
    long n = sys_read(0, buf, sizeof(buf));
    while (n > 0) {
        long nwrt = n > nb ? nb : n;

        sys_write(1, buf, nwrt);
        nb -= nwrt;
        if (nb <= 0) {
            break;
        }
        n = sys_read(0, buf, sizeof(buf));
    }

    sys_close(0);
    sys_close(1);
    return 0;
}
