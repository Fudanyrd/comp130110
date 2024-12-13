#include "syscall.h"

int main(int argc, char **argv)
{
    int id = sys_fork();
    if (id < 0) {
        sys_print("fork FAIL", 9);
        return 1;
    }

    if (id == 0) {
        char *argv[5] = {
            "/bin/echo", "foo", "bar", "baz", NULL
        };
        sys_execve(argv[0], argv);
        sys_print("execve FAIL", 11);
        sys_exit(2);
    } else {
        int code;
        sys_wait(&code);
        if (code == 0) {
            sys_print("execve OK", 9);
        }
    }

    return 0;
}
