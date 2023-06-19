#include "param.h"
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"

//
// This file contains copyin_new() and copyinstr_new(), the
// replacements for copyin and coyinstr in vm.c.
//

static struct stats {
  int ncopyin;
  int ncopyinstr;
} stats;

int
statscopyin(char *buf, int sz) {
  int n;
  n = snprintf(buf, sz, "copyin: %d\n", stats.ncopyin);
  n += snprintf(buf+n, sz, "copyinstr: %d\n", stats.ncopyinstr);
  return n;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  struct proc *p = myproc();
  uint64 va;

  int npages;
  npages = u2kvmmap(pagetable, p->kpgtable, srcva, len, PTE_R);
  if (npages < 0)
    return -1;

  // srcva + len < srcva test overflow for uint64
  if (srcva >= p->sz || srcva+len >= p->sz || srcva+len < srcva) {
    goto ret;
    return -1;
  }

  memmove((void *) dst, (void *)srcva, len);
  stats.ncopyin++;   // XXX lock

ret:
  va = PGROUNDDOWN(srcva);
  uvmunmap(p->kpgtable, va, npages, 0);
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  struct proc *p = myproc();
  char *s = (char *) srcva;
  int npages;
  uint64 np, va;
  if (max == 0) return -1;

  npages = u2kvmmap(pagetable, p->kpgtable, srcva, 1, PTE_R);
  if (npages < 0) {
    return -1;
  }

  np = PGROUNDUP(srcva + 1);
  stats.ncopyinstr++;   // XXX lock
  for(int i = 0; i < max && srcva + i < p->sz; i++){
    if (srcva + i == np) {
      int npg = u2kvmmap(pagetable, p->kpgtable, np, 1, PTE_R);
      if (npg < 0) {
        goto error;
      }
      np += PGSIZE;
      npages += npg;
    }

    dst[i] = s[i];
    if(s[i] == '\0') {
      uint64 va = PGROUNDDOWN(srcva);
      uvmunmap(p->kpgtable, va, npages, 0);
      return 0;
    }
  }
error:
  va = PGROUNDDOWN(srcva);
  uvmunmap(p->kpgtable, va, npages, 0);

  return -1;
}
