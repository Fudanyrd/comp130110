// necho: network echo
// prints to console what it receives.

#include "syscall.h"

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
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

    static char buf[4096];
    sys_write(fd, "Hello networking!\n", 19);

    // receive a message from server, and exit.
    // this just checks that network's state.
    long nb = sys_read(fd, buf, sizeof(buf));
    if (nb < 0) {
        sys_write(2, "socket broken\n", 15);
        sys_close(fd);
        return 1;
    }
    sys_write(1, buf, nb);

    // bye.
    sys_close(fd);
    return 0;
}
