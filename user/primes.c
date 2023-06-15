#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int is_prime(int n) {
    for (int i = 2; i < n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

void childothers(int leftfd) {
    int prime, ret;
    ret = read(leftfd, (void*)&prime, sizeof(int));
    if (ret <= 0) {
        close(leftfd);
        exit(3);
    }

    printf("prime %d\n", prime);

    int pipright[2];
    if (pipe(pipright) < 0) {
        fprintf(2, "pipe error\n");
        exit(1);
    }

    int childid = fork();
    if (childid > 0) {
        close(pipright[0]);
    } else if (childid == 0) {
        close(leftfd);
        close(pipright[1]);
        childothers(pipright[0]);
    } else {
        close(leftfd);
        close(pipright[0]);
        close(pipright[1]);
        fprintf(2, "%s fork error\n", __func__);
        exit(2);
    }

    while (1) {
        ret = read(leftfd, (void*)&prime, sizeof(int));
        if (ret <= 0) {
            close(leftfd);
            close(pipright[1]);
            wait(0);
            exit(0);
        }

        ret = write(pipright[1], (void*)&prime, sizeof(int));
        if (ret < 0) {
            close(leftfd);
            close(pipright[1]);
            wait(0);
            exit(3);
        }
    }
}

void child1(int parentfd) {
    int num, ret;
    int pip2[2];
    if (pipe(pip2) < 0) {
        fprintf(2, "%s pipe error\n", __func__);
        exit(1);
    }

    int childid = fork();
    if (childid > 0) {
        close(pip2[0]);
    } else if (childid == 0) {
        close(pip2[1]);
        childothers(pip2[0]);
    } else {
        fprintf(2, "%s fork error\n", __func__);
        close(parentfd);
        close(pip2[0]);
        close(pip2[1]);
        exit(2);
    }

    while (1) {
        ret = read(parentfd, (void*)&num, sizeof(int));
        if (ret <= 0) {
            close(parentfd);
            close(pip2[1]);
            wait(0);
            exit(3);
        }
        if (is_prime(num)) {
            write(pip2[1], (void*)&num, sizeof(int));
        }
    }
}

int
main()
{
    int pip[2];
    if (pipe(pip) < 0) {
        fprintf(2, "%$s pipe error\n", __func__);
        exit(1);
    }

    int child = fork();
    if (child > 0) {
        close(pip[0]);
        int ret;
        for (int i = 2; i <= 35; i++) {
            ret = write(pip[1], (void*)&i, sizeof(int));
            if (ret < 0) {
                close(pip[1]);
                fprintf(2, "%s write failed\n", __func__);
                exit(3);
            }
        }
        close(pip[1]);
        wait(0);
    } else if (child == 0) {
        close(pip[1]);
        child1(pip[0]);
    } else {
        fprintf(2, "%s fork error\n", __func__);
        exit(2);
    }

    exit(0);
}