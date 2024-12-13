extern int sys_mkdir(const char *path);
extern int sys_chdir(const char *path);
extern void sys_print(const char *s, unsigned long len);

int main(int argc, char **argv)
{
    if (sys_chdir("/home/")) {
        sys_print("cd /home FAIL", 13);
        return 1;
    }

    if (sys_mkdir("a/")) {
        sys_print("mkdir a FAIL", 12);
        return 1;
    }
    sys_chdir("./a");
    sys_chdir("..");

    if (sys_mkdir("b/")) {
        sys_print("mkdir b FAIL", 12);
    }
    sys_chdir("./b");
    sys_chdir("..");

    sys_print("OK", 2);
    return 0;
}
