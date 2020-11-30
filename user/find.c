
// find all the files in a directory tree with a specific name

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

void
find(char *path, char *filename)
{
    char buf[512], *p, *name;
    int fd, i, sign, length;
    struct dirent de;
    struct stat st;

    //printf("find path: %s filename: %s\n", path, filename);

    length = strlen(path);
    sign = 0;
    for (i=0;i<length;i++) {
        if (path[i] == '/') {
            sign = i;
        }
    }

    name = path;
    if (sign > 0)
        name = path + sign + 1;
    //printf("path: %p name: %p\n", path, name);
    //printf("path : %s name: %s\n", path, name);

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type)
    {
    case T_FILE:
    case T_DEVICE:
        //printf("%s file or device\n", path);
        if (strcmp(name, filename) == 0) {
            printf("%s\n", path);
        }
        break;
    case T_DIR:
        //printf("%s dir\n", path); //
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) { // path + '\0' + name + '\0'
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de)) {
            //printf("read de.inum: %d de.name: %s\n", de.inum, de.name);
            if (de.inum == 0) // ?
                continue;
            memmove(p, de.name, DIRSIZ); // TODO try memcpy
            p[DIRSIZ] = 0;

            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                continue;
            }
            //printf("buf: %s\n", buf);

            find(buf, filename);
        }
    default:
        break;
    }

   close(fd);
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2, "find [path] [filenamt]");
        exit(1);
    }

    find(argv[1], argv[2]);

    exit(0);
}