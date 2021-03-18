#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    // system call

    if(p->killed)
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
    // time tick
    if (which_dev == 2) {
      p->lticks++;
      if (p->handler != 0 && p->lticks == p->ticks) {
          p->lticks = 0;
          switchtrapframe(p->retrapframe, p->trapframe);
          p->trapframe->epc = (uint64)p->handler;
      }
        
    }
  } else {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if(p->killed)
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  usertrapret();
}

//
// save register
//
void
switchtrapframe(struct trapframe *dst, struct trapframe *src)
{
  dst->kernel_satp = src->kernel_satp;
  dst->kernel_sp = src->kernel_sp;
  dst->kernel_trap = src->kernel_trap;
  dst->epc = src->epc;
  dst->kernel_hartid = src->kernel_hartid;
  dst->ra = src->ra;
  dst->sp = src->sp;
  dst->gp = src->gp;
  dst->tp = src->tp;
  dst->t0 = src->t0;
  dst->t1 = src->t1;
  dst->t2 = src->t2;
  dst->s0 = src->s0;
  dst->s1 = src->s1;
  dst->a0 = src->a0;
  dst->a1 = src->a1;
  dst->a2 = src->a2;
  dst->a3 = src->a3;
  dst->a4 = src->a4;
  dst->a5 = src->a5;
  dst->a6 = src->a6;
  dst->a7 = src->a7;
  dst->s2 = src->s2;
  dst->s3 = src->s3;
  dst->s4 = src->s4;
  dst->s5 = src->s5;
  dst->s6 = src->s6;
  dst->s7 = src->s7;
  dst->s8 = src->s8;
  dst->s9 = src->s9;
  dst->s10 = src->s10;
  dst->s11 = src->s11;
  dst->t3 = src->t3;
  dst->t4 = src->t4;
  dst->t5 = src->t5;
  dst->t6 = src->t6;
}



// void
// restoreregister(void)
// {
  // struct proc *p = myproc();
// 
  // p->trapframe->kernel_satp = p->retrapframe->kernel_satp;
  // p->trapframe->kernel_sp = p->retrapframe->kernel_sp;
  // p->trapframe->kernel_trap = p->retrapframe->kernel_trap;
  // p->trapframe->epc = p->retrapframe->epc;
  // p->trapframe->kernel_hartid = p->retrapframe->kernel_hartid;
  // p->trapframe->ra = p->retrapframe->ra;
  // p->trapframe->sp = p->retrapframe->sp;
  // p->trapframe->gp = p->retrapframe->gp;
  // p->trapframe->tp = p->retrapframe->tp;
  // p->trapframe->t0 = p->retrapframe->t0;
  // p->trapframe->t1 = p->retrapframe->t1;
  // p->trapframe->t2 = p->retrapframe->t2;
  // p->trapframe->s0 = p->retrapframe->s0;
  // p->trapframe->s1 = p->retrapframe->s1;
  // p->trapframe->a0 = p->retrapframe->a0;
  // p->trapframe->a1 = p->retrapframe->a1;
  // p->trapframe->a2 = p->retrapframe->a2;
  // p->trapframe->a3 = p->retrapframe->a3;
  // p->trapframe->a4 = p->retrapframe->a4;
  // p->trapframe->a5 = p->retrapframe->a5;
  // p->trapframe->a6 = p->retrapframe->a6;
  // p->trapframe->a7 = p->retrapframe->a7;
  // p->trapframe->s2 = p->retrapframe->s2;
  // p->trapframe->s3 = p->retrapframe->s3;
  // p->trapframe->s4 = p->retrapframe->s4;
  // p->trapframe->s5 = p->retrapframe->s5;
  // p->trapframe->s6 = p->retrapframe->s6;
  // p->trapframe->s7 = p->retrapframe->s7;
  // p->trapframe->s8 = p->retrapframe->s8;
  // p->trapframe->s9 = p->retrapframe->s9;
  // p->trapframe->s10 = p->retrapframe->s10;
  // p->trapframe->s11 = p->retrapframe->s11;
  // p->trapframe->t3 = p->retrapframe->t3;
  // p->trapframe->t4 = p->retrapframe->t4;
  // p->trapframe->t5 = p->retrapframe->t5;
  // p->trapframe->t6 = p->retrapframe->t6;
// }

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));

  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // jump to alarm handler
  // if (p->handler != 0 && (p->lticks == p->ticks) && p->handling == 0) {
  //   p->handling = 1;
  //   // save register
  //   saveregister();
  //   // printf("call handler, jump to :%p, raw: %p\n", (uint64 *)p->handler, (uint64 *)p->trapframe->epc);
  //   p->trapframe->epc = (uint64)p->handler;
  // }
  // // alram handler done, return to user code
  // else if (p->handler != 0 && p->handlerdone == 1){
  //   p->handlerdone = 0;
  //   p->handling = 0;
  //   p->lticks = 0;
  //   // restore register
  //   restoreregister();
  //   // printf("done, return to :%p\n", (uint64 *)p->trapframe->epc);
  // } 
  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if(cpuid() == 0){
      clockintr();
    }
    
    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

