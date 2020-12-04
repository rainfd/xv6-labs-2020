// read from stdout and run command

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

void readline(char *buf, int max)
{
    int i, cc;
    char c;

    for (i = 0; i + 1 < max;)
    {
        cc = read(0, &c, 1);
        if (cc < 1)
            break;
        if (c == '\n' || c == '\r' || c == '\0')
            break;
        else
            buf[i++] = c;
    }
    buf[i] = '\0';
}

int main(int argc, char *argv[])
{
    int i, pid;
    // char *start, *end, buf[4096];
    // bug: input > 4096

    // char buf[512], argvs[MAXARG][512];
    char buf[512], **argvs;

    if (argc < 2)
    {
        fprintf(2, "[command] | xargs [command]\n");
        exit(1);
    }
    if (argc > MAXARG)
    {
        fprintf(2, "too many parameters!\n");
        exit(1);
    }

    argvs = malloc(sizeof(*argvs) * (argc + 1));
    for (i = 1; i < argc; i++)
    {
        argvs[i - 1] = malloc(strlen(argv[i]) + 1);
        strcpy(argvs[i - 1], argv[i]);
    }
    argvs[argc - 1] = malloc(512);
    argvs[argc] = 0;

    while (1)
    {
        readline(buf, 512);
        if (strlen(buf) <= 0)
            break;

        strcpy(argvs[argc - 1], buf);

        pid = fork();
        if (pid == 0)
        {
            exec(argvs[0], argvs);
            exit(0);
        }
    }

    wait(0);

    // for (i = 0; i < argc + 2; i++)
    // printf("argc %d %s\n", i, argvs[i]);
    // free(argvs[i]);
    // free(argvs);

    exit(0);
}