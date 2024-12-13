// fork's behavior is parent first.
// so the output of this test should be:
//
// [USER] from parent
// [USER] from child
// [USER] p
// [USER] c

#include "syscall.h"
static char buf[4];

int main(int argc, char **argv) {
    int i = 0;
    buf[1] = 0;

    if (sys_fork() == 0) {
        sys_print("from child", 10);
        if (sys_fork() == 0) {
            buf[0] = 'c';
            sys_print(buf, 1);
        } else {
            buf[0] = 'p';
            sys_print(buf, 1);
        }

        // child exit.
        sys_exit(0);
    } else {
        sys_print("from parent", 11);
    }

    return 0;
}
