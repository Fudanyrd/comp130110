// Test the combination of fork() and wait.
// if goes as expected, the program shall output:
// [USER] from child
// [USER] from paren
// [USER] child id OK
// [USER] exitcode OK
#include "syscall.h"
#define CODE 0x1234

int main(int argc, char **argv)
{
    int id = sys_fork();
    if (id < 0) {
        sys_print("fork FAIL", 9);
        sys_exit(1);
    }

    int code;
    int chd;
    if (id == 0) {
        sys_print("from child", 10);
        sys_exit(CODE);
    } else {
        chd = sys_wait(&code);
        sys_print("from parent", 10);

        if (chd == id) {
            sys_print("child id OK", 11);
        }
        if (code == CODE) {
            sys_print("exitcode OK", 11);
        }
    }
}
