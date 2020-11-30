// ping-pong a byte between two process

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char buf[512];

void
pingpong(void)
{
    int p1[2], p2[2];
    int pid;

    if (pipe(p1) == -1) {
        fprintf(2, "create pipe failed!");
        exit(1);
    }
    if (pipe(p2) == -1) {
        fprintf(2, "create pipe failed!");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        fprintf(2, "fork new process failed!");
        exit(1);
    }

    if (pid == 0) {
        close(p1[1]);
        while (read(p1[0], buf, sizeof(buf)) > 0)
            fprintf(1, "%d: received %s\n", getpid(), buf);
        close(p1[0]);

        close(p2[0]);
        write(p2[1], "pong", 4);
        close(p2[1]);
    } else {
        close(p1[0]);
        write(p1[1], "ping", 4);
        close(p1[1]);

        close(p2[1]);
        while (read(p2[0], &buf, 4) > 0)
            fprintf(1, "%d: received %s\n", getpid(), buf);
        close(p2[0]);
    }
}

int
main(void)
{
    pingpong();
    exit(0);
}