// this test checks the file system's read, write and 
// remove functionality. It first create a relatively
// large file /home/tmp.txt, then write data to it.
// Then, it tries to read from the file to make sure
// persistence. After read/write succeed, we remove
// it.

#include "syscall.h"
// a large buffer.
static char buf[8192];

const char *data = "abcdefghijklmnopqrstuvwxyz";
const static usize len = 26;

int main(int argc, char **argv)
{
    // init the buffer.
    sys_print("write TEST", 10);
    for (usize i = 0; i < sizeof(buf); i++) {
        buf[i] = data[i % len];
    }

    // open a file and write buffer.
    int fd = sys_open("/home/tmp.txt", O_CREATE | O_READ | O_WRITE);
    if (sys_write(fd, buf, sizeof(buf)) != sizeof(buf)) {
        sys_print("write FAIL", 10);
        return 1;
    }
    sys_close(fd);

    // reopen and read the data into buffer.
    // this should put offset to 0.
    fd = sys_open("/home/tmp.txt", O_READ);
    if (sys_read(fd, buf, sizeof(buf)) != sizeof(buf)) {
        sys_print("read FAIL", 9);
        return 1;
    }
    sys_close(fd);

    // check the read data.
    for (usize i = 0; i < sizeof(buf); i++) {
        if (buf[i] != data[i % len]) {
            sys_print("check FAIL", 10);
            return 1;
        }
    }

    // remove this file.
    if (sys_unlink("/home/tmp.txt") != 0) {
        sys_print("unlink FAIL", 11);
        return 1;
    }
    fd = sys_open("/home/tmp.txt", O_READ | O_WRITE);
    if (fd >= 0) {
        sys_print("open should FAIL", 16);
        return 1;
    }

    // ok, test passed.
    sys_print("OK", 2);
    return 0;
}
