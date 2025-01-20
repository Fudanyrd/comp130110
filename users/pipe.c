#include "syscall.h"

int main(int argc, char **argv)
{
    // open a pipe
    int pipfd[2];
    if (sys_pipe((int *)pipfd)) {
        return 1;
    }

    int id = sys_fork();

    if (id < 0) {
        return 1;
    }

    if (id == 0) {
        // child
        char *cat[] = { "/bin/cat", "/home/compile.sh", NULL };

        sys_dup2(pipfd[1], 1);
        sys_close(pipfd[0]);
        sys_close(pipfd[1]);
        sys_execve(cat[0], cat);
        sys_exit(1);
    } else {
        // parent
        char *wc[] = {
            "/bin/wc",
            NULL,
        };

        sys_dup2(pipfd[0], 0);
        sys_close(pipfd[1]);
        sys_close(pipfd[0]);
        sys_execve(wc[0], wc);
        sys_exit(1);
    }
}
