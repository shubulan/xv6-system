// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"
#include "kalloc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

char* memlockname[NCPU] = {
       "kmem0", "kmem1", "kmem2", "kmem3",
       "kmem4", "kmem5", "kmem6", "kmem7"};
void
kinit()
{
  int cuid = cpuid();

  initlock(&cpus[cuid].mem.lock, memlockname[cuid]);
  if (cuid == 0) {
    freerange(end, (void*)PHYSTOP);
  }
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct cpu *cu;
  struct run *r;
  push_off();
  cu = mycpu();
  pop_off();

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&cu->mem.lock);
  r->next = cu->mem.freelist;
  cu->mem.freelist = r;
  release(&cu->mem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct cpu* cu;
  struct run *r;
  push_off();
  cu = mycpu();
  pop_off();

  acquire(&cu->mem.lock);
  r = cu->mem.freelist;
  if(r)
    cu->mem.freelist = r->next;
  release(&cu->mem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  else { // steal mem
    for (int i = 0; i < NCPU; i++) {
      struct cpu *tcpu = &cpus[i];
      if (tcpu == cu || !tcpu->mem.freelist) {
        continue;
      }
      acquire(&tcpu->mem.lock);
      r = tcpu->mem.freelist;
      if(r)
        tcpu->mem.freelist = r->next;
      release(&tcpu->mem.lock);
      break;
    }
  }

  return (void*)r;
}
