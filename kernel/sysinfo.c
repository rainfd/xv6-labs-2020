#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

// Get the number of bytes of free memory, 
// and get the number of processed 
// whose state is not UNUSED.
int
systeminfo(uint64 addr)
{
    struct proc *p = myproc();
    struct sysinfo info;

    info.freemem = kcount();
    info.nproc = nproc();
    if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
        return -1;
    return 0;
}


uint64 
sys_sysinfo(void)
{
    uint64 info; // user pointer to struct sysinfo
    if (argaddr(0, &info) < 0)
        return -1;
    return systeminfo(info);
}