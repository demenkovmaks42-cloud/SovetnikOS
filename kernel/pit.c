#include "pit.h"
#include "pic.h"

#define PIT_CH0   0x40
#define PIT_CMD   0x43

static volatile uint32_t ticks = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}

void pit_init(uint32_t hz) {
    uint32_t divisor = PIT_FREQ / hz;
    outb(PIT_CMD, 0x36);   /* канал 0, lobyte/hibyte, mode 3 */
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
    ticks = 0;
}

uint32_t pit_ticks(void) { return ticks; }

/* Вызывается из IRQ0 */
void pit_tick(void) { ticks++; }
