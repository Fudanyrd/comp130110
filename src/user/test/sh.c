#include "syscall.h"
#include <stdbool.h>

static char buf[512];

static void execve(char *cmd);

int main(int argc, char **argv)
{
    sys_write(1, "> ", 2);
    isize nrd = sys_read(0, buf, sizeof(buf));
    while (nrd > 0) {
        // make this cmd as null-terminiated
        buf[nrd - 1] = 0;
        execve(buf);
        sys_write(1, "> ", 2);
        nrd = sys_read(0, buf, sizeof(buf));
    }

    sys_close(0);
    return 0;
}

static void execve(char *cmd)
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

    int id = sys_fork();
    if (id < 0) {
        // fail
        return;
    }
    if (id == 0) {
        sys_execve(argv[0], argv);
        sys_print("execve FAIL", 11);
        sys_exit(1);
    } else {
        // block execution
        sys_wait(&id);
    }
}
