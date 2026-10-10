#include "pit.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43

volatile uint32_t pit_ticks = 0;

static uint32_t pit_frequency = PIT_DEFAULT_FREQUENCY;

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

void pit_init(uint32_t hz)
{
    uint32_t divisor;

    if (hz == 0) {
        hz = PIT_DEFAULT_FREQUENCY;
    }

    divisor = PIT_BASE_FREQUENCY / hz;

    if (divisor == 0) {
        divisor = 1;
    }

    if (divisor > 65535u) {
        divisor = 65535u;
    }

    pit_frequency = hz;
    pit_ticks = 0;

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFFu));
}

void pit_tick(void)
{
    pit_ticks++;
}
