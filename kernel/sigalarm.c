#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"

int sig_status = 0;

void
sigalarm(int interval, void handler(void))
{
  int pid;

  if (interval == 0) {
    sig_status = 0;
    return;
  }

  if ((pid = fork()) < 0) {
      panic("fork failed");
  }
  sig_status = 1;
  if (pid == 0) {
    while (sig_status) {

      for (int i = 0; i < interval * 500000; i++) {
          if (sig_status)
            return;
          asm volatile("nop"); // avoid compiler optimizing away loop
      }

      handler();
    }
  }
}

uint64
sys_sigalarm(void)
{
  int interval;
  uint64 addr;

  if (argint(0, &interval) < 0)
    return -1;
  if (argaddr(0, &addr) < 0)
    return -1;
  sigalarm(interval, (void (*)(void))addr);
  return 0;
}


uint64
sys_sigreturn(void)
{
  sig_status = 0;
  return 0;
}
