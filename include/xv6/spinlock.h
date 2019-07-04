/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-06-22 00:05:43
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-07-04 18:30:30
 * @ Description:
 */


#if !defined(__INCLUDE_XV6_SPINLOCK_H)
#define __INCLUDE_XV6_SPINLOCK_H

// Mutual exclusion lock.
struct spinlock
{
  uint locked; // Is the lock held?

  // For debugging:
  char *name;      // Name of lock.
  struct cpu *cpu; // The cpu holding the lock.
  uint pcs[10];    // The call stack (an array of program counters)
                   // that locked the lock.
};

#endif // __INCLUDE_XV6_SPINLOCK_H
