#include "pit.h"
#include "types.h"

#define PIT_COMMAND  0x43
#define PIT_CHANNEL0 0x40

static volatile uint32_t ticks = 0;

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

    if (hz == 0)
        hz = 100;

    /*
     * Частота входного сигнала PIT:
     * 1 193 182 Гц.
     */
    divisor = PIT_FREQ / hz;

    if (divisor == 0)
        divisor = 1;

    if (divisor > 65535)
        divisor = 65535;

    /*
     * PIT:
     * Channel 0
     * Access: low byte + high byte
     * Mode 3: square wave
     */
    outb(PIT_COMMAND, 0x36);

    /* Младший байт делителя */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));

    /* Старший байт делителя */
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    ticks = 0;
}

void pit_tick(void)
{
    ticks++;
}

uint32_t pit_ticks(void)
{
    return ticks;
}
