struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  uint hash;
  uint lticks;
  struct buf *prev; // in bucket list 
  struct buf *next;
  uchar data[BSIZE];
};

