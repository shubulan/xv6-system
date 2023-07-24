// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct BBucket {
  struct spinlock lock;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf buf[NBUF];
} bbucket[13]; // 13 bucket and last one(14) are zero list

char* bucketnames[] = {
  "bcache.bucket1", "bcache.bucket2", "bcache.bucket3", "bcache.bucket4", "bcache.bucket5", "bcache.bucket6", "bcache.bucket7",
  "bcache.bucket8", "bcache.bucket9", "bcache.bucket10", "bcache.bucket11", "bcache.bucket12", "bcache.bucket13", "bcache.zero"
};

void
binit(void)
{
  struct buf *b;
  for (int i = 0; i < 13; i++) {
    initlock(&bbucket[i].lock, bucketnames[i]);
    for (b = bbucket[i].buf; b < bbucket[i].buf + NBUF; b++) {
      initsleeplock(&b->lock, "buffer");
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b, *p;
  uint32 hash = dev * 263 + blockno;
  hash %= 13;
  struct BBucket *bkt = &bbucket[hash];


  acquire(&bkt->lock);

  // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  for(b = bkt->buf; b != bkt->buf + NBUF; b++){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bkt->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // // it's ok to release this lock:
  // //   1. if two process run this func to get different block, it's totally ok;
  // //   2. if two process run this func to get same block, it will break for allocating two cache to the same block;
  // //      but 2 is not possible for this lab;
  // release(&bkt->lock);

  b = 0;
  for(p = bkt->buf; p != bkt->buf + NBUF; p++){
    if (p->refcnt != 0) continue;
    if (!b || b->lticks > p->lticks) {
      b = p;
    }
  }

  if (b == 0)
    panic("bget: no buffers");
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->hash = hash;
  release(&bkt->lock);

  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  struct BBucket *bkt = &bbucket[b->hash];

  acquire(&bkt->lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->lticks = ticks;
  }
  release(&bkt->lock);
}

void
bpin(struct buf *b) {
  struct BBucket* bkt = &bbucket[b->hash];
  acquire(&bkt->lock);
  b->refcnt++;
  release(&bkt->lock);
}

void
bunpin(struct buf *b) {
  struct BBucket* bkt = &bbucket[b->hash];
  acquire(&bkt->lock);
  b->refcnt--;
  release(&bkt->lock);
}


