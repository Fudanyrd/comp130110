
static void Puts(const char *s);
extern void sys_print(const char *s, unsigned long len);
extern long sys_write(int fd, const char *buf, unsigned long size);
extern int sys_exit(int code);

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        Puts(argv[i]);
    }
    sys_write(1, "\n", 1);
    sys_exit(0);
}

static void Puts(const char *s)
{
    unsigned long len = 0;
    for (len = 0; s[len] != 0; len++) {
    }

    sys_write(1, s, len);
    sys_write(1, " ", 1);
    return;
}
