// show how long system has been running

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    // int time = uptime();
    printf("%d\n", uptime());
    exit(0);
}