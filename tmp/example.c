#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * I forgot where I saw this example. Perhaps on stackoverflow.com.
 */

int main(int argc, char *argv[])
{
    int pd[2]; //Pipe descriptor
    pipe(pd);
    int pid = fork();
    if (pid < 0)
        perror("Something failed on trying to create a child process!\n");
    else if (pid == 0) { //Child
        dup2(pd[0], 0);
        close(pd[0]);
        close(pd[1]);
        execlp("wc", "wc", (char *)NULL);
        fprintf(stderr, "Failed to execute 'wc'\n");
        exit(1);
    } else { //Parent
        dup2(pd[1], 1);
        close(pd[0]);
        close(pd[1]);
        execlp("echo", "echo", "foo", "bar", (char *)NULL);
        fprintf(stderr, "Failed to execute 'wc'\n");
        exit(1);
    }
}
