// word count. 
// print (line, word, byte)

// This is needed to check the behavior of execve.
// Since this executable is larger than a page!
// Must handle correctly.

#include "syscall.h"
#include <stdbool.h>

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

static u32 line, word, byte;

static void wc(int fd) {
    line = word = byte = 0;

    bool printable(char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') 
               || (ch >= '0' && ch <= '9');
    }

    static char buf[512];
    long nrd = sys_read(fd, buf, sizeof(buf));

    bool blank = true;
    bool empty = true;
    while (nrd > 0) {
        for (long i = 0; i < nrd; i++) {
            if (blank) {
                if (printable(buf[i])) {
                    blank = false;
                    word++;
                }
            } else {
                if (!printable(buf[i])) {
                    blank = true;
                }
            }

            if (buf[i] == '\n') {
                line++;
                empty = true;
            } else {
                empty = false;
            }
        }

        byte += (u32)nrd;
        nrd = sys_read(fd, buf, sizeof(buf));
    }

    line += empty ? 0 : 1;
    printu(1, line);
    printu(1, word);
    printu(1, byte);
    sys_write(1, "\n", 1);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        wc(0);
    } else {
        for (int i = 1; i < argc; i++) {
            int fd = sys_open(argv[i], O_READ);
            if (fd >= 0) {
                wc(fd);
                sys_close(fd);
            }
        }
    }

    return 0;
}
