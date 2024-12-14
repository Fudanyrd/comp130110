#include "syscall.h"

static void printu(int fd, u32 val) {
    static char buf[16];
    int n = 0;
    while (val > 0) {
        buf[n++] = '0' + val % 10;
        val /= 10;
    }

    for (int i = 0; i < n / 2; i++) {
        char l = buf[i];
        char r = buf[n - 1 - i];
        buf[i] = r;
        buf[n - i - 1] = l;
    }

    buf[n] = ' ';
    sys_write(fd, buf, n + 1);
}

static void fdstat(int fd) {
    static InodeEntry entry;
    if (sys_fstat(fd, &entry) != 0) {
        sys_write(1, "stat fail\n", 10);
        return;
    }

    // inode type
    switch (entry.type) {
    case (INODE_DEVICE): {
        sys_write(1, "device ", 7);
        break;
    }
    case (INODE_REGULAR): {
        sys_write(1, "file   ", 7);
        break;
    }
    case (INODE_DIRECTORY): {
        sys_write(1, "dir    ", 7);
        break;
    }
    default: {
        sys_write(1, "???    ", 7);
        break;
    }
    }

    // size 
    printu(1, entry.num_bytes);
    // nlink
    printu(1, (u32)entry.num_links);

    sys_write(1, "\n", 1);
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        int fd = sys_open(argv[i], O_READ);
        if (fd < 0) {
            continue;
        }
        fdstat(fd);
        sys_close(fd);
    }

    return 0;
}
