// this test is copied from lab of previous year.
// url:
// https://github.com/FDUCSLG/OS-23Fall-FDU/blob/lab-final-unfinished/src/user/usertests/main.c
// if no "FAIL" is printed to console, congrats! Test Pass.

#include "syscall.h"

static char buf[8192];
static char name[3];

static void opentest(void);
static void writetest(void);
static void writetestbig(void);
static void createtest(void);

int main(int argc, char **argv)
{
    // do open test
    sys_print("open TEST", 9);
    opentest();

    // writetest
    sys_print("write TEST", 10);
    writetest();

    // writebig test
    sys_print("wrt large TEST", 14);
    writetestbig();

    // create test
    sys_print("create test", 11);
    createtest();

    sys_print("OK", 2);
    return 0;
}

static void opentest(void)
{
    int fd = sys_open("/init", O_READ);
    if (fd < 0) {
        sys_print("open FAIL", 9);
        return;
    }

    fd = sys_open("doesnotexist", O_READ);
    if (fd >= 0) {
        sys_print("open should FAIL", 16);
        return;
    }
    sys_print("open PASS", 9);
}


static void writetest(void)
{
    int fd;
    fd = sys_open("small", O_CREATE | O_READ | O_WRITE);

    if (fd < 0) {
        sys_print("creat FAIL", 10);
        return;
    }

    for (int i = 0; i < 100; i++) {
        if (sys_write(fd, "aaaaaaaaaa", 10) != 10) {
            sys_print("write FAIL", 10);
            break;
        }
        if (sys_write(fd, "bbbbbbbbbb", 10) != 10) {
            sys_print("write FAIL", 10);
            break;
        }
    }

    sys_close(fd);
    sys_print("write PASS", 10);

    // read test
    fd = sys_open("small", O_READ);
    if (sys_read(fd, buf, 2000) != 2000) {
        sys_print("read FAIL", 9);
        return;
    }
    sys_close(fd);
    sys_print("read PASS", 9);

    if (sys_unlink("small") < 0) {
        sys_print("unlink FAIL", 11);
    } else {
        sys_print("unlink PASS", 11);
    }
}

static void writetestbig(void)
{
    // create a big file that is equal to max bytes.
    int fd;
    fd = sys_open("large", O_CREATE | O_READ | O_WRITE);

    if (fd < 0) {
        sys_print("creat FAIL", 10);
        return;
    }

    u32 *pt = (u32 *)buf;
    for (u32 i = 0; i < 256; i++) {
        pt[0] = i;
        if (sys_write(fd, buf, 512) != 512) {
            sys_print("write FAIL", 10);
            return;
        }
    }
    sys_close(fd);

    fd = sys_open("large", O_READ);
    for (u32 i = 0; i < 256; i++) {
        if (sys_read(fd, buf, 512) != 512) {
            sys_print("read FAIL", 9);
            return;
        }
        u32 header = pt[0];
        if (header != i) {
            sys_print("persist FAIL", 12);
            return;
        }
    }

    sys_close(fd);

    // unlink the file
    if (sys_unlink("large") < 0) {
        sys_print("unlink FAIL", 10);
    } else {
        sys_print("wrt large PASS", 14);
    }
}

static void createtest(void)
{
    int fd;

    name[0] = 'a';
    name[2] = 0;
    for (int i = 0; i < 52; i++) {
        name[1] = i + '0'; 
        fd = sys_open(name, O_CREATE | O_READ | O_WRITE);
        if (fd < 0) {
            sys_print("open FAIL", 9);
            return;
        }
        sys_close(fd);
    }

    for (int i = 0; i < 52; i++) {
        name[1] = i + '0';
        if (sys_unlink(name) < 0) {
            sys_print("unlink FAIL", 10);
            return;
        }
    }

    sys_print("create PASS", 11);
}
