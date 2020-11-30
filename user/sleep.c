// Simple sleep. Sleep n seconds.

#include "kernel/types.h"
#include "user/user.h" 

//void sleep(int);

int
main(int argc, char *argv[])
{
    int n;

    if(argc < 1) {
        fprintf(2, "usage: sleep x\n");
    }

    n = atoi(argv[1]);
    fprintf(1, "sleep %d seconds\n", n);
    sleep(n);

    exit(0);
}