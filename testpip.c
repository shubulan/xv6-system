#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
// #include "kernel/types.h"
// #include "user/user.h"

int main()
{
    char *byte = (char*)malloc(10);
    int pipePC[2];
    if (pipe(pipePC) < 0) {
        fprintf(stderr, "pipe error\n");
        exit(1);
    }

    int child = fork();
    if (child > 0) {
        sleep(3);
        close(pipePC[0]);
        close(pipePC[1]);
        free(byte);
        wait(0);
    } else if (child == 0) {
        close(pipePC[1]);
        printf("%d: child reading %s\n", getpid(), byte);
        int ret = read(pipePC[0], byte, 5);
        printf("%d: received ret %d\n", getpid(), ret);
        close(pipePC[0]);
    } else {
        fprintf(stderr, "fork error\n");
        exit(2);
    }
    exit(0);
}

