
// in fs/file1206.h
#define O_READ 0x1
#define O_WRITE 0x2
#define O_CREATE 0x4

static int cat(const char *path);
extern void sys_print(const char *s, unsigned long len);
extern int sys_open(const char *s, int flags);
extern int sys_close(int fd);
extern long sys_read(int fd, char *buf, unsigned long size);

int main(int argc, char **argv) 
{
    cat("/home/README.txt");
    for (int i = 1; i < argc; i++) {
        cat(argv[i]);
    }
    return 0;
}

static int cat(const char *path)
{
    static char buf[512];
    int fd = sys_open(path, O_READ);
    if (fd < 0) {
        sys_print("Open failure", 12);
        return -1;
    }

    long nread = sys_read(fd, buf, sizeof(buf));
    while (nread > 0) {
        sys_print((char *)buf, nread);
        nread = sys_read(fd, buf, sizeof(buf));
    }
    return 0;
}

