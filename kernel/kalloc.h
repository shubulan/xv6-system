#ifndef __kalloc__
#define __kalloc__
struct kmem{
  struct spinlock lock;
  struct run *freelist;
};
#endif