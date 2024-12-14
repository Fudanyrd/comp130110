#include "syscall.h"
#include <stdbool.h>

static char buf[512];

static void system(char *cmd);
static void Execve(char *exe, char **argv);
static int strcmp(const char *a, const char *b);
static char *strcpy(char *dst, const char *src);

int main(int argc, char **argv)
{
    sys_write(1, "> ", 2);
    isize nrd = sys_read(0, buf, sizeof(buf));
    while (nrd > 0) {
        // make this cmd as null-terminiated
        buf[nrd - 1] = 0;
        system(buf);
        sys_write(1, "> ", 2);
        nrd = sys_read(0, buf, sizeof(buf));
    }

    sys_close(0);
    return 0;
}

static void system(char *cmd)
{
    // split
    static char *argv[16];
    int narg = 0;
    argv[0] = NULL;
    _Bool blank = true;

    for (int i = 0; cmd[i] != 0; i++) {
        if (cmd[i] != ' ' && cmd[i] != '\t') {
            if (blank) {
                blank = false;
                argv[narg++] = &cmd[i];
            }
        } else {
            if (!blank) {
                cmd[i] = 0;
                blank = true;
            }
        }
    }

    argv[narg] = NULL;
    if (narg == 0) {
        // empty cmd
        return;
    }

    // built-in command cd
    if (strcmp(argv[0], "cd") == 0) {
        if (sys_chdir(argv[1]) != 0) {
            sys_print("chdir FAIL", 10);
        }
        return;
    }

    int id = sys_fork();
    if (id < 0) {
        // fail
        return;
    }
    if (id == 0) {
        Execve(argv[0], argv);
    } else {
        // block execution
        sys_wait(&id);
    }
}

static int strcmp(const char *a, const char *b)
{
    int i = 0;
    for (; a[i] != 0 && b[i] != 0; i++) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }

    return (int)a[i] - (int)b[i];
}

// safely do execve.
static void Execve(char *exe, char **argv)
{
    // search path option 0: cwd
    sys_execve(exe, argv);

    // search path option 1: /bin/
    static char pth[64];
    char *pt = strcpy(pth, "/bin/");
    strcpy(pt, exe);
    sys_execve((const char *)pth, argv);

    sys_print("execve FAIL", 11);
    sys_exit(1);
}

// Returns the dst after copy(*dst = 0).
static char *strcpy(char *dst, const char *src)
{
    while (*src != 0) {
        *dst = *src;
        src++;
        dst++;
    }

    *dst = 0;
    return dst;
}
