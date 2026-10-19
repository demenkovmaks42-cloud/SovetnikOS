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
static idt_ptr_t idtp;

extern void idt_load(uint32_t);

#define DECLARE_EXCEPTION(n) extern void exception_stub_##n(void)
DECLARE_EXCEPTION(0);  DECLARE_EXCEPTION(1);  DECLARE_EXCEPTION(2);
DECLARE_EXCEPTION(3);  DECLARE_EXCEPTION(4);  DECLARE_EXCEPTION(5);
DECLARE_EXCEPTION(6);  DECLARE_EXCEPTION(7);  DECLARE_EXCEPTION(8);
DECLARE_EXCEPTION(9);  DECLARE_EXCEPTION(10); DECLARE_EXCEPTION(11);
DECLARE_EXCEPTION(12); DECLARE_EXCEPTION(13); DECLARE_EXCEPTION(14);
DECLARE_EXCEPTION(15); DECLARE_EXCEPTION(16); DECLARE_EXCEPTION(17);
DECLARE_EXCEPTION(18); DECLARE_EXCEPTION(19); DECLARE_EXCEPTION(20);
DECLARE_EXCEPTION(21); DECLARE_EXCEPTION(22); DECLARE_EXCEPTION(23);
DECLARE_EXCEPTION(24); DECLARE_EXCEPTION(25); DECLARE_EXCEPTION(26);
DECLARE_EXCEPTION(27); DECLARE_EXCEPTION(28); DECLARE_EXCEPTION(29);
DECLARE_EXCEPTION(30); DECLARE_EXCEPTION(31);
#undef DECLARE_EXCEPTION

extern void irq0_stub(void);  extern void irq1_stub(void);
extern void irq2_stub(void);  extern void irq3_stub(void);
extern void irq4_stub(void);  extern void irq5_stub(void);
extern void irq6_stub(void);  extern void irq7_stub(void);
extern void irq8_stub(void);  extern void irq9_stub(void);
extern void irq10_stub(void); extern void irq11_stub(void);
extern void irq12_stub(void); extern void irq13_stub(void);
extern void irq14_stub(void); extern void irq15_stub(void);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].base_low = (uint16_t)(base & 0xFFFFu);
    idt[num].base_high = (uint16_t)((base >> 16) & 0xFFFFu);
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_init(void)
{
    static void (*const exceptions[32])(void) = {
        exception_stub_0, exception_stub_1, exception_stub_2, exception_stub_3,
        exception_stub_4, exception_stub_5, exception_stub_6, exception_stub_7,
        exception_stub_8, exception_stub_9, exception_stub_10, exception_stub_11,
        exception_stub_12, exception_stub_13, exception_stub_14, exception_stub_15,
        exception_stub_16, exception_stub_17, exception_stub_18, exception_stub_19,
        exception_stub_20, exception_stub_21, exception_stub_22, exception_stub_23,
        exception_stub_24, exception_stub_25, exception_stub_26, exception_stub_27,
        exception_stub_28, exception_stub_29, exception_stub_30, exception_stub_31
    };
    static void (*const irqs[16])(void) = {
        irq0_stub, irq1_stub, irq2_stub, irq3_stub,
        irq4_stub, irq5_stub, irq6_stub, irq7_stub,
        irq8_stub, irq9_stub, irq10_stub, irq11_stub,
        irq12_stub, irq13_stub, irq14_stub, irq15_stub
    };
    uint32_t i;

    idtp.limit = (uint16_t)(sizeof(idt) - 1u);
    idtp.base = (uint32_t)&idt;

    for (i = 0; i < 256u; ++i)
        idt_set_gate((uint8_t)i, 0, 0, 0);

    /* Install CPU exception handlers first: vectors 0..31. */
    for (i = 0; i < 32u; ++i)
        idt_set_gate((uint8_t)i, (uint32_t)exceptions[i], 0x08, 0x8E);

    /* Remapped legacy PIC IRQs: vectors 32..47. */
    for (i = 0; i < 16u; ++i)
        idt_set_gate((uint8_t)(32u + i), (uint32_t)irqs[i], 0x08, 0x8E);

    idt_load((uint32_t)&idtp);
}
