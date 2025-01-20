#include "syscall.h"

static char counter = 'a';

void child_fn()
{
    char message[] = { '[', 'c', 'h', 'i', 'l', 'd', ']', ' ', 0 };
    message[8] = counter;
    sys_print((char *)message, 9);
    sys_exit(0);
}

int main(int argc, char **argv)
{
    sys_print("[parent] 0", 11);

    for (int i = 0; i < 10; i++) {
        int id = sys_fork();
        if (id < 0) {
            sys_exit(1);
        }
        if (id == 0) {
            child_fn();
        } else {
            sys_wait(NULL);
        }
        counter++;
    }
    sys_exit(0);
}
