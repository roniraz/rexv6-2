// The local APIC manages internal (non-I/O) interrupts.
// See Chapter 8 & Appendix C of Intel processor manual volume 3.

#include "xv6/param.h"
#include "xv6/types.h"
#include "xv6/defs.h"
#include "xv6/date.h"
#include "xv6/memlayout.h"
#include "xv6/traps.h"
#include "xv6/mmu.h"
#include "arch/x86/x86.h"

#include "drivers/apic/lapic.h"
#include "drivers/timer/timer.h"

volatile uint *lapic; // Initialized in mp.c

//PAGEBREAK!
void lapicw(int index, int value)
{
  lapic[index] = value;
  lapic[ID]; // wait for write to finish, by reading
}

void lapicinit(void)
{
  if (!lapic)
    return;

  // Enable local APIC; set spurious interrupt vector.
  lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

  // Enable lapic timer
  lapictimer_init();

  // Disable logical interrupt lines.
  lapicw(LINT0, MASKED);
  lapicw(LINT1, MASKED);

  // Disable performance counter overflow interrupts
  // on machines that provide that interrupt entry.
  if (((lapic[VER] >> 16) & 0xFF) >= 4)
    lapicw(PCINT, MASKED);

  // Map error interrupt to IRQ_ERROR.
  lapicw(ERROR, T_IRQ0 + IRQ_ERROR);

  // Clear error status register (requires back-to-back writes).
  lapicw(ESR, 0);
  lapicw(ESR, 0);

  // Ack any outstanding interrupts.
  lapicw(EOI, 0);

  // Send an Init Level De-Assert to synchronise arbitration ID's.
  lapicw(ICRHI, 0);
  lapicw(ICRLO, BCAST | INIT | LEVEL);
  while (lapic[ICRLO] & DELIVS)
    ;

  // Enable interrupts on the APIC (but not on the processor).
  lapicw(TPR, 0);
}

int lapicid(void)
{
  if (!lapic)
    return 0;
  return lapic[ID] >> 24;
}

// Acknowledge interrupt.
void lapiceoi(void)
{
  if (lapic)
    lapicw(EOI, 0);
}

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
void microdelay(int us)
{
}

#define CMOS_PORT 0x70
#define CMOS_RETURN 0x71

// Start additional processor running entry code at addr.
// See Appendix B of MultiProcessor Specification.
void lapicstartap(uchar apicid, uint addr)
{
  int i;
  ushort *wrv;

  // "The BSP must initialize CMOS shutdown code to 0AH
  // and the warm reset vector (DWORD based at 40:67) to point at
  // the AP startup code prior to the [universal startup algorithm]."
  outb(CMOS_PORT, 0xF); // offset 0xF is shutdown code
  outb(CMOS_PORT + 1, 0x0A);
  wrv = (ushort *)P2V((0x40 << 4 | 0x67)); // Warm reset vector
  wrv[0] = 0;
  wrv[1] = addr >> 4;

  // "Universal startup algorithm."
  // Send INIT (level-triggered) interrupt to reset other CPU.
  lapicw(ICRHI, apicid << 24);
  lapicw(ICRLO, INIT | LEVEL | ASSERT);
  microdelay(200);
  lapicw(ICRLO, INIT | LEVEL);
  microdelay(100); // should be 10ms, but too slow in Bochs!

  // Send startup IPI (twice!) to enter code.
  // Regular hardware is supposed to only accept a STARTUP
  // when it is in the halted state due to an INIT.  So the second
  // should be ignored, but it is part of the official Intel algorithm.
  // Bochs complains about the second one.  Too bad for Bochs.
  for (i = 0; i < 2; i++)
  {
    lapicw(ICRHI, apicid << 24);
    lapicw(ICRLO, STARTUP | (addr >> 12));
    microdelay(200);
  }
}

#define CMOS_STATA 0x0a
#define CMOS_STATB 0x0b
#define CMOS_UIP (1 << 7) // RTC update in progress

#define SECS 0x00
#define MINS 0x02
#define HOURS 0x04
#define DAY 0x07
#define MONTH 0x08
#define YEAR 0x09

static uint
cmos_read(uint reg)
{
  outb(CMOS_PORT, reg);
  microdelay(200);

  return inb(CMOS_RETURN);
}

static void
fill_rtcdate(struct rtcdate *r)
{
  r->second = cmos_read(SECS);
  r->minute = cmos_read(MINS);
  r->hour = cmos_read(HOURS);
  r->day = cmos_read(DAY);
  r->month = cmos_read(MONTH);
  r->year = cmos_read(YEAR);
}

// qemu seems to use 24-hour GWT and the values are BCD encoded
void cmostime(struct rtcdate *r)
{
  struct rtcdate t1, t2;
  int sb, bcd;

  sb = cmos_read(CMOS_STATB);

  bcd = (sb & (1 << 2)) == 0;

  // make sure CMOS doesn't modify time while we read it
  for (;;)
  {
    fill_rtcdate(&t1);
    if (cmos_read(CMOS_STATA) & CMOS_UIP)
      continue;
    fill_rtcdate(&t2);
    if (memcmp(&t1, &t2, sizeof(t1)) == 0)
      break;
  }

  // convert
  if (bcd)
  {
#define CONV(x) (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
    CONV(second);
    CONV(minute);
    CONV(hour);
    CONV(day);
    CONV(month);
    CONV(year);
#undef CONV
  }

  *r = t1;
  r->year += 2000;
}
