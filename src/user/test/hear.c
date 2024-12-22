// hear: 
// listening from port, print any incoming message to
// the console. This program will not exit unless the
// socket is broken.

#include "syscall.h"

#ifndef MAXCOUNT
#define MAXCOUNT 20
#endif 

int atoi(const char *s)
{
    int n;

    n = 0;
    while ('0' <= *s && *s <= '9')
        n = n * 10 + *s++ - '0';
    return n;
}

#define MAKE_IP_ADDR(a, b, c, d) \
    (((u32)a << 24) | ((u32)b << 16) | ((u32)c << 8) | (u32)d)

extern int sys_socket(u32 raddr, u16 lport, u16 rport);

int main(int argc, char **argv)
{
    if (argc < 2) {
        sys_write(2, "No dst port provided.\n", 23);
        return 1;
    }

    // qemu's IP addr: 10.0.2.2
    int fd = sys_socket(MAKE_IP_ADDR(10, 0, 2, 2), 2000, atoi(argv[1]));
    if (fd < 0) {
        sys_write(2, "open socket failed.\n", 21);
        return 1;
    }
    sys_write(fd, "This is qemu!\n", 15);

    static char buf[4096];

    // receive a message from server, and exit.
    // this just checks that network's state.
    u64 count = 0;
    long nb = sys_read(fd, buf, sizeof(buf));
    while (nb > 0) {
        sys_write(1, buf, nb);
        nb = sys_read(fd, buf, sizeof(buf));
        count++;
        if (count >= MAXCOUNT) {
            break;
        }
    }

    sys_exit(0);
}
