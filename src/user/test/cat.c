
// in fs/file1206.h
#define O_READ 0x1
#define O_WRITE 0x2
#define O_CREATE 0x4

static void cat(int fd);
extern void sys_print(const char *s, unsigned long len);
extern int sys_open(const char *s, int flags);
extern int sys_close(int fd);
extern long sys_read(int fd, char *buf, unsigned long size);
extern long sys_write(int fd, const char *buf, unsigned long size);

int main(int argc, char **argv)
{
    if (argc == 1) {
        cat(0);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        int fd = sys_open(argv[i], O_READ);
        if (fd > 0) {
            cat(fd);
            sys_close(fd);
        }
    }
    return 0;
}

static void cat(int fd)
{
    static char buf[512];
    long nrd = sys_read(fd, buf, sizeof(buf));
    while (nrd > 0) {
        if (sys_write(1, buf, nrd) < 0) {
            break;
        }
        nrd = sys_read(fd, buf, sizeof(buf));
    }
    return;
}
