// init program starts shell, then
// waits for it to terminate, which will not happen.

#include "syscall.h"

int main(int argc, char **argv)
{
    int id = sys_fork();

    if (id < 0) {
        // dead loop
        sys_print("fork FAIL", 9);
        goto end;
    }
    if (id == 0) {
        // child, execute shell.
        char *argv[2] = { "/bin/sh", NULL };
        sys_execve(argv[0], argv);
        sys_print("start shell FAIL", 16);
        sys_exit(1);
    }

end:
    // since the kernel puts all zombies to root prog(this),
    // we should use wait() to free all zombie procs.
    for (;;) {
        sys_wait(&id);
    }
}
