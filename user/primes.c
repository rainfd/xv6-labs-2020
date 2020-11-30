// output primes from 2 to 35

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
primes(int ppipe[2])
{
    int cpipe[2];
    int pid, status;
    int n, prime, num;

    // 1. print the first num
    close(ppipe[1]);
    while (1) {
        n = read(ppipe[0], &prime, sizeof(int));
        if (n > 0) {
            fprintf(1, "prime %d\n", prime);
            break;
        }
        if (n <= 0) {
            // parent pipe close
            return;
        }
    }

    // 2. create child process and pipe
    if (pipe(cpipe) == -1) {
        fprintf(2, "create pipe failed!");
        exit(1);
    }
    pid = fork();
    if (pid < 0) {
        fprintf(2, "fork new process failed!");
        exit(1);
    }
   
    // child
    if (pid == 0) {
        primes(cpipe);
    } 
    
    // parent
    // 3. continues output filter number
    if (pid != 0) {
        while(1) {
            n = read(ppipe[0], &num, sizeof(int));
            if (n > 0) {
                if (num % prime != 0) {
                    write(cpipe[1], &num, sizeof(int));
                }
            } else {
                // parent pipe close
                // => close child pipe
                close(cpipe[1]);
                wait(&status);
                return;
            }
        }
   }
}

int
main(void)
{
    int i, pid, status;
    int ppipe[2];

    if (pipe(ppipe) == -1) {
        fprintf(2, "create pipe failed!");
        exit(1);
    } 
    pid = fork();
    if (pid < 0) {
        fprintf(2, "fork new process failed!");
        exit(1);
    }

    if (pid == 0) {
        primes(ppipe);
    } else {
        close(ppipe[0]);
        for (i=2;i<35;i++) {
            write(ppipe[1], &i, sizeof(int));
        }
        close(ppipe[1]);
        wait(&status);
    }

    exit(0);
}