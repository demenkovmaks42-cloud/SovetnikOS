#ifndef PIT_H
#define PIT_H

#include "types.h"

#define PIT_BASE_FREQUENCY    1193182u
#define PIT_DEFAULT_FREQUENCY 100u

extern volatile uint32_t pit_ticks;

void pit_init(uint32_t hz);
void pit_tick(void);

#endif
