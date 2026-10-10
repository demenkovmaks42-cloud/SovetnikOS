#include "idt.h"
#include "vga.h"
#include "pic.h"

typedef struct {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t   idtp;

extern void idt_load(uint32_t);

/* ассемблерные обёртки (см. boot/isr_stubs.asm) */
extern void irq0_stub(void);
extern void irq1_stub(void);
extern void irq2_stub(void);
extern void irq3_stub(void);
extern void irq4_stub(void);
extern void irq5_stub(void);
extern void irq6_stub(void);
extern void irq7_stub(void);
extern void irq8_stub(void);
extern void irq9_stub(void);
extern void irq10_stub(void);
extern void irq11_stub(void);
extern void irq12_stub(void);
extern void irq13_stub(void);
extern void irq14_stub(void);
extern void irq15_stub(void);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

void idt_init(void) {
    idtp.limit = sizeof(idt_entry_t) * 256 - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < 256; i++)
        idt_set_gate(i, 0, 0, 0);

    /* IRQ0..15 → векторы 32..47, селектор 0x08, флаг 0x8E */
    idt_set_gate(32, (uint32_t)irq0_stub,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1_stub,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2_stub,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3_stub,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4_stub,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5_stub,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6_stub,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7_stub,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8_stub,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9_stub,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10_stub, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11_stub, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12_stub, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13_stub, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14_stub, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15_stub, 0x08, 0x8E);

    idt_load((uint32_t)&idtp);
}
