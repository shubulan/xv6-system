#include "syscall.h"
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64 sys_sigalarm(void) {
  struct proc *p = myproc();
  argint(0, &(p->ticks_interval));
  argaddr(1, &(p->ticks_handle));
  return 0;
}

uint64 sys_sigreturn(void) {
  struct proc *p = myproc();
  *(p->trapframe) =*(p->ticks_trapframe);
  p->ticks_handle_calling = 0;
  return 0;
}