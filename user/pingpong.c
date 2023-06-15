#include "kernel/types.h"
#include "user/user.h"

int main()
{
    char *byte = (char*)malloc(10);
    int pipePC[2];
    int pipeCP[2];
    if (pipe(pipePC) < 0) {
        fprintf(2, "pipe error\n");
        exit(1);
    }
    if (pipe(pipeCP) < 0) {
        fprintf(2, "pipe error\n");
        exit(1);
    }

    int child = fork();
    if (child > 0) {
        close(pipePC[0]);
        close(pipeCP[1]);
        write(pipePC[1], "ping", 5);
        read(pipeCP[0], byte, 5);
        printf("%d: received %s\n", getpid(), byte);
    } else if (child == 0) {
        close(pipePC[1]);
        close(pipeCP[0]);
        read(pipePC[0], byte, 5);
        printf("%d: received %s\n", getpid(), byte);
        write(pipeCP[1], "pong", 5);
    } else {
        fprintf(2, "fork error\n");
        exit(2);
    }
    exit(0);
}

