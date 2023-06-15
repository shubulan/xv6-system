#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int readline(char* buff) {
    int bx = 0, ret;
    while (bx != 1024) {
        ret = read(0, &(buff[bx]), 1);
        if (ret <= 0) {
            return ret;
        }
        if (buff[bx] == '\n') {
            buff[bx] = 0;
            return bx;
        }
        bx++;
    }
    return -2;
}

int
main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(2, "Usage: xargs [options] [command [args]]\n"); 
        exit(1);
    }

    int i = 1, multi = 0, cargc = 0, eargc = 0;
    char* cargv[MAXARG] = {0};
    if (strcmp(argv[i], "-n") == 0) {
        i++;
        multi = atoi(argv[i++]);
        if (multi < 0) {
            fprintf(2, "invalid [-n [num]]\n"); 
            exit(2);
        }
    }

    for (; i < argc; i++) {
        cargv[cargc++] = argv[i];
    }

    char buff[1024];

    do {
        eargc = 0;
        int ret;
        ret = readline(buff);
        if (ret <= 0) {
            if (ret < 0) fprintf(2, "readline error");
            exit(2);
        }
        int flag = 0;
        for (; eargc < ret; eargc++) {
            if (!flag) {
                cargv[cargc + eargc] = &buff[eargc];
                flag = 1;
            }
            if (buff[i] == ' ') {
                flag = 0;
                buff[i] = 0;
            }
        }
        int child = fork();
        if (child > 0) {
        } else if (child == 0) {
            ret = exec(cargv[0], cargv);
            fprintf(2, "exec error %s\n", cargv[0]);
            exit(2);
        } else {
            fprintf(2, "fork error\n");
            exit(3);
        }
        wait(0);
    } while(multi);

    exit(0);
}