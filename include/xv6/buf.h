#if !defined(__INCLUDE_XV6_BUF_H)
#define __INCLUDE_XV6_BUF_H

#include "xv6/sleeplock.h"

struct buf {
  int flags;
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;//  number need to exec
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue
  uchar data[BSIZE];
};
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk


#endif // __INCLUDE_XV6_BUF_H
