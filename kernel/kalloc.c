// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

uint16 pa_ref[PHYPGCOUNT];
void
kinit()
{
  // init ref 1 to be free
  for (int i = 0; i < PHYPGCOUNT; i++) pa_ref[i] = 1;
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
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
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.

  r = (struct run*)pa;

  acquire(&kmem.lock);
  pa_ref[PHYPGIDX(pa)]--;
  if (pa_ref[PHYPGIDX(pa)] != 0) {
    release(&kmem.lock);
    return;
  }
  memset(pa, 1, PGSIZE); // push off junky time.
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  // memset(pa, 1, PGSIZE); // push off junky time. bug: pointer r->next been junky
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    if (pa_ref[PHYPGIDX(r)]) {
      panic("kalloc: ref not 0\n");
    }
    pa_ref[PHYPGIDX(r)] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void
kpaaddref(uint64 pa) {
  acquire(&kmem.lock);
  pa_ref[PHYPGIDX(pa)]++;
  release(&kmem.lock);
}

int
kpadecref(uint64 pa) {
  uint64 oldref;
  acquire(&kmem.lock);
  oldref = pa_ref[PHYPGIDX(pa)];
  if (oldref < 1) {
    panic("decref");
  }
  pa_ref[PHYPGIDX(pa)]--;
  release(&kmem.lock);
  return oldref;
}

int
kgetparef(uint64 pa) {
  uint64 ref;
  acquire(&kmem.lock);
  ref = pa_ref[PHYPGIDX(pa)];
  release(&kmem.lock);
  return ref;
}