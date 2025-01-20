#include "syscall.h"
#define N 512

int main(int argc, char **argv) {
    for (int i = 0; i < N; i++) {
        int id = sys_fork();
        if (id < 0) {
            sys_write(2, "fork error\n", 12);
            sys_exit(1);
        }

        if (id == 0) {
            // child
            sys_exit(0);
        }
    }

    for (int i = 0; i < N; i++) {
        int exitcode;
        if (sys_wait(&exitcode) < 0) {
            sys_write(2, "wait error\n", 12);
            sys_exit(1);
        }
        if (exitcode != 0) {
            sys_write(2, "incorrect exit code\n", 21);
            sys_exit(1);
        }
    }

    sys_write(1, "OK\n", 4);
    return 0;
}
