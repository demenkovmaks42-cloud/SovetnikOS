#include "pic.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t r;
    __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}
static inline void io_wait(void) { outb(0x80, 0); }

void pic_init(void) {
    /* ICW1: начало инициализации */
    outb(PIC1_CMD,  0x11); io_wait();
    outb(PIC2_CMD,  0x11); io_wait();
    /* ICW2: смещение векторов */
    outb(PIC1_DATA, 0x20); io_wait();   /* IRQ0..7  → int 32..39 */
    outb(PIC2_DATA, 0x28); io_wait();   /* IRQ8..15 → int 40..47 */
    /* ICW3: связь master-slave */
    outb(PIC1_DATA, 0x04); io_wait();
    outb(PIC2_DATA, 0x02); io_wait();
    /* ICW4: 8086 mode */
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();
    /* Маскируем всё, кроме IRQ0 (таймер) и IRQ1 (клавиатура) */
    outb(PIC1_DATA, 0xFC);   /* 1111 1100 — разрешены IRQ0 и IRQ1 */
    outb(PIC2_DATA, 0xFF);   /* всё запрещено */
}

void pic_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

void pic_mask(uint8_t irq) {
    uint16_t port;
    uint8_t val;
    if (irq < 8) { port = PIC1_DATA; }
    else         { port = PIC2_DATA; irq -= 8; }
    val = inb(port) | (1 << irq);
    outb(port, val);
}

void pic_unmask(uint8_t irq) {
    uint16_t port;
    uint8_t val;
    if (irq < 8) { port = PIC1_DATA; }
    else         { port = PIC2_DATA; irq -= 8; }
    val = inb(port) & ~(1 << irq);
    outb(port, val);
}
