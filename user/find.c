#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

char cur_path[1024];
int id;
void shiftpath(char* path) {
    if (id != 0 && id < 1023) cur_path[id++] = '/';
    int len = strlen(path);
    if (id + len > 1023) {
        len = 1023 - id;
    }
    memmove(cur_path + id, path, len);
    id += len;
    cur_path[id] = '\0';
}
void unshiftpath() {
    for (int i = id - 1; i >= 0; i--) {
        if (cur_path[i] == '/') {
            cur_path[i] = 0;
            id = i;
            return;
        }
    }
    cur_path[0] = 0;
    id = 0;
}

int match(char*, char*);

void find(int dir_fd, char* tar_name) {
    struct dirent de;
    int cur_fd;
    while (read(dir_fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
            continue;
        }

        shiftpath(de.name);
        if (match(tar_name, de.name)) {
            printf(cur_path);
            printf("\n");
        }

        if ((cur_fd = open(cur_path, 0)) < 0) {
            printf("open error %s\n", cur_path);
            unshiftpath();
            continue;
        }
        struct stat st;
        if (fstat(cur_fd, &st) < 0) {
            fprintf(2, "find: can't stat %s\n", cur_path);
            close(cur_fd);
            unshiftpath();
            continue;
        }
        if (st.type == T_DIR) {
            find(cur_fd, tar_name);
        }
        close(cur_fd);
        unshiftpath();
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(2, "Usage: find [dir] [filename]\n");
        exit(1);
    }
    int fd;
    if ((fd = open(argv[1], 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", argv[1]);
        exit(2);
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: can't stat %s\n", argv[1]);
        close(fd);
        exit(3);
    }
    if (st.type != T_DIR) {
        fprintf(2, "find: %s is not a directory\n", argv[1]);
        close(fd);
        exit(4);
    }
    shiftpath(argv[1]);
    find(fd, argv[2]);
    unshiftpath();

    close(fd);


    exit(0);
}

// Regexp matcher from Kernighan & Pike,
// The Practice of Programming, Chapter 9.

int matchhere(char*, char*);
int matchstar(int, char*, char*);

int
match(char *re, char *text)
{
  if(re[0] == '^')
    return matchhere(re+1, text);
  do{  // must look at empty string
    if(matchhere(re, text))
      return 1;
  }while(*text++ != '\0');
  return 0;
}

// matchhere: search for re at beginning of text
int matchhere(char *re, char *text)
{
  if(re[0] == '\0')
    return 1;
  if(re[1] == '*')
    return matchstar(re[0], re+2, text);
  if(re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if(*text!='\0' && (re[0]=='.' || re[0]==*text))
    return matchhere(re+1, text+1);
  return 0;
}

// matchstar: search for c*re at beginning of text
int matchstar(int c, char *re, char *text)
{
  do{  // a * matches zero or more instances
    if(matchhere(re, text))
      return 1;
  }while(*text!='\0' && (*text++==c || c=='.'));
  return 0;
}