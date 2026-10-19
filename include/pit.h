#ifndef PIT_H
#define PIT_H

#include "types.h"

#define PIT_FREQ 1193182u

void pit_init(uint32_t hz);
void pit_tick(void);
uint32_t pit_ticks(void);

#endif
