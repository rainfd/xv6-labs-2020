#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"

int
sigalarm(int ticks, void (*handler)())
{
  struct proc *p = myproc();
  p->ticks = ticks;
  p->lticks = 0;
  p->handler = handler;
  return 0;
}

uint64
sys_sigalarm(void)
{
  int ticks;
  uint64 addr;

  if (argint(0, &ticks) < 0)
    return -1;
  if (argaddr(0, &addr) < 0)
    return -1;

  struct proc *p = myproc();
  if (p->retrapframe == 0) {
    if((p->retrapframe = (struct trapframe *)kalloc()) == 0){
      panic("retrapframe kalloc");
    }
  }

  sigalarm(ticks, (void (*)())addr);
  return 0;
}


uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  switchtrapframe(p->trapframe, p->retrapframe);
  return 0;
}
