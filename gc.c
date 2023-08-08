#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct Node{
    int64_t val;
    struct Node *next;
} Node;

#define MAX_BUFF 4096
void* from_buf;
void* to_buf;
void* empty_start;
Node* head;
int gc_cnt;


void gc() {
    gc_cnt++;
    to_buf = malloc(MAX_BUFF);

    // copy head;
    void* empty_gc = to_buf;
    memcpy(empty_gc, head, sizeof(Node));
    head = empty_gc;
    empty_gc += sizeof(Node);

    // copy reachable node
    int a = 0;
    Node* p = head;
    while (p->next) { 
        a++;
        memcpy(empty_gc, p->next, sizeof(Node));
        p->next = empty_gc;
        empty_gc += sizeof(Node);
        p = p->next;
    }
    printf("gc: copy %d nodes\n", a);
    // single list no loop

    // switch  from <--> to
    void *tmp = from_buf;
    from_buf = to_buf;
    empty_start = empty_gc;
    free(tmp);
}

Node* __NewNode() {
    Node* ret;
    if (empty_start + sizeof(Node) <= from_buf + MAX_BUFF) {
        ret = empty_start;
        empty_start += sizeof(Node);
        return ret;
    }
    return 0;
}
Node* NewNode() {
    Node *ret = 0;
    if ((ret = __NewNode()))
        return ret;

    gc();
    return __NewNode();
}

void initContext() {
    from_buf = malloc(MAX_BUFF);
    empty_start = from_buf;
}

void test_gc() {
    Node* tmp;
    int max_len, loop_max, i, dt, at;
    max_len = MAX_BUFF / sizeof(Node);
    dt = 0;
    at = 0;
    loop_max = MAX_BUFF;
    i = 0;
    srand(42);
    printf("max len = %d\n", max_len);
    while ((at - dt) != max_len && i++ < loop_max) {
        int op = rand() % 3;
        if (op == 0 && head) {
            printf("deleting %d\n", at - dt);
            head = head->next;
            dt++;
        } else {
            printf("adding %d\n", at - dt);
            tmp = NewNode();
            if (!tmp) break;
            tmp->val = loop_max;
            tmp->next = head;
            head = tmp;
            at++;
        }
    }
    printf("gc cnt %d\n", gc_cnt);
    if (at - dt != max_len) {
        printf("failed: loop %d expire %d get %d   %d %d\n", i, max_len, at - dt, at, dt);
    } else {
        printf("success: loop %d expire %d get %d  %d %d\n", i, max_len, at - dt, at, dt);
    }
}

int main() {
    initContext();
    test_gc();
    return 0;
}