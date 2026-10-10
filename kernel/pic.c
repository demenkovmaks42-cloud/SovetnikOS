#include "pic.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI   0x20

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void io_wait(void)
{
    outb(0x80, 0);
}

void pic_init(void)
{
    /* Сохраняем исходные маски IRQ */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* Начинаем инициализацию обоих PIC */
    outb(PIC1_CMD, 0x11);
    io_wait();
    outb(PIC2_CMD, 0x11);
    io_wait();

    /* Переносим IRQ в диапазоны 32–39 и 40–47 */
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    /* Настраиваем каскадирование */
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();

    /* Режим совместимости с 8086 */
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    /* Восстанавливаем исходные маски */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_eoi(uint8_t irq)
{
    if (irq >= 16) {
        return;
    }

    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }

    outb(PIC1_CMD, PIC_EOI);
}

void pic_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t bit;
    uint8_t value;

    if (irq >= 16) {
        return;
    }

    if (irq < 8) {
        port = PIC1_DATA;
        bit = irq;
    } else {
        port = PIC2_DATA;
        bit = (uint8_t)(irq - 8);
    }

    value = inb(port);
    value = (uint8_t)(value | (uint8_t)(1u << bit));
    outb(port, value);
}

void pic_unmask(uint8_t irq)
{
    uint16_t port;
    uint8_t bit;
    uint8_t value;

    if (irq >= 16) {
        return;
    }

    if (irq < 8) {
        port = PIC1_DATA;
        bit = irq;
    } else {
        port = PIC2_DATA;
        bit = (uint8_t)(irq - 8);
    }

    value = inb(port);
    value = (uint8_t)(value & (uint8_t)~(1u << bit));
    outb(port, value);

    /* Для IRQ ведомого PIC нужен и IRQ2 главного */
    if (irq >= 8) {
        value = inb(PIC1_DATA);
        value = (uint8_t)(value & (uint8_t)~(1u << 2));
        outb(PIC1_DATA, value);
    }
}
