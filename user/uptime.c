#include "kernel/types.h"
#include "user/user.h"

int
main() {
    int ret = uptime();
    if (ret < 0) {
        fprintf(2, "uptime: uptime\n");
        exit(1);
    }
    printf("%d\n", ret);
    exit(0);
}