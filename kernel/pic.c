#include "pic.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define PIC_EOI      0x20

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

void pic_init(void)
{
    uint8_t mask1;
    uint8_t mask2;

    /* Сохраняем текущие маски прерываний */
    mask1 = 0xFF;
    mask2 = 0xFF;

    /* Начинаем перенастройку PIC */
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    /* Переносим IRQ в диапазон 32–47 */
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    /* Связываем контроллеры */
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    /* Режим 8086 */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* Включаем только IRQ0–IRQ2 на главном PIC */
    outb(PIC1_DATA, (uint8_t)(mask1 & 0xF8));
    outb(PIC2_DATA, mask2);
}

void pic_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }

    outb(PIC1_COMMAND, PIC_EOI);
}
