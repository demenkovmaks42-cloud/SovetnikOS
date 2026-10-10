#ifndef PIT_H
#define PIT_H

#include "types.h"

#define PIT_FREQ 1193182

void     pit_init(uint32_t hz);
uint32_t pit_ticks(void);

#endif
