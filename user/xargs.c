// read from stdout and run command

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/fs.h"
#include "user/user.h"

int
xargs()
{
    ;
}

int
main(int argc, char *argv[])
{
    int i, n, length;
    char *arg, buf[4096];

    if (argc > MAXARG)
        fprintf(2, "too many parameters!\n");

    // read from stdin 
    // fork -> exec argv + stdin
    while (n = read(0, buf, 4096) > 0) {
        length = strlen(buf);

        strchr(buf, "\n")

        for (i=0;i<length;i++) {
            if (buf[i] == "\n") {
                //memmove(arg, buf, i);
                //arg[i] = "\0";
                //arg++ = "\0";
            } else {
                //*arg++ = *buf++;
            }
        }
    }

    exit(0);
}