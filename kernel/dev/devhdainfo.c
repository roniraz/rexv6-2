#include "xv6/types.h"
#include "xv6/defs.h"
#include "xv6/param.h"
#include "xv6/traps.h"
#include "xv6/spinlock.h"
#include "xv6/sleeplock.h"
#include "xv6/fs.h"
#include "xv6/file.h"
#include "xv6/buf.h"
#include "xv6/memlayout.h"
#include "xv6/mmu.h"
#include "xv6/proc.h"
#include "xv6/x86.h"

static struct
{
  struct spinlock lock;
  int locking;
  uint r;
} cons;

//int useable_capcity, used_capcity;
int usablesizes[NPARTITIONS] = {0};
int usedsizes[NPARTITIONS] = {0};

int block_off;

int hdainforead(struct inode *ip, char *dst, uint off, uint n)
{
  iunlock(ip);
  for (int i = 0; i < NPARTITIONS; i++)
  {
    cprintf("Partition %d:\n", i);
    cprintf(" usable capcity : %d.%dKB\n", usablesizes[i] / KBSIZE, (usablesizes[i] * 10 / KBSIZE) % 10);
    cprintf(" used capcity : %d.%d KB\n", usedsizes[i] / KBSIZE, (usedsizes[i] * 10 / KBSIZE) % 10);
    cprintf(" set block off %d\n", block_off);
  }
  ilock(ip);
  return 0;
}
int hdainfowrite(struct inode *ip, char *cbuf, uint off, uint n)
{
  iunlock(ip);
  int input_off = 0, i;
  if (cons.r == 0)
  {
    for (i = 0; i < n && cbuf[i] != 10; i++)
    {
      if (cbuf[i] >= 0 + '0' && cbuf[i] <= 9 + '0')
      {
        input_off *= 10;
        input_off += cbuf[i] - '0';
      }
      else
      {
        cprintf("write failure\n");
        ilock(ip);
        return n;
      }
    }
    block_off = input_off;
    cprintf("set block off %d\n", block_off);
  }
  ilock(ip);
  return n;
}

void hdainfoinit(void)
{
  initlock(&cons.lock, "devhdainfo");

  memset(usablesizes, 0, sizeof(usablesizes));
  memset(usedsizes, 0, sizeof(usedsizes));
  devsw[NDEVHDAINFO][MDEVHDAINFO].write = hdainfowrite;
  devsw[NDEVHDAINFO][MDEVHDAINFO].read = hdainforead;

  cons.locking = 1;
}
