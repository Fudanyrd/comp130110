extern int sys_mkdir(const char *path);
extern int sys_chdir(const char *path);
extern void sys_print(const char *s, unsigned long len);

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (sys_mkdir(argv[i]) < 0) {
            sys_print("mkdir FAIL", 10);
        }
    }
    return 0;
}
