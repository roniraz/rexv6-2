#if !defined(__INCLUDE_DRIVERS_TIMER_TIMER_H)
#define __INCLUDE_DRIVERS_TIMER_TIMER_H

#include "drivers/apic/lapic.h"


#if defined(__cplusplus)
extern "C"
{
#endif

    void lapictimer_init(void);

#if defined(__cplusplus)
}
#endif

#endif // __INCLUDE_DRIVERS_TIMER_TIMER_H
