#include "gdt.h"

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

static gdt_entry_t gdt[3];
static gdt_ptr_t   gdtp;

extern void gdt_load(uint32_t);

static void set_gate(int num, uint32_t base, uint32_t limit,
                     uint8_t access, uint8_t gran) {
    gdt[num].base_low    = base & 0xFFFF;
    gdt[num].base_mid    = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = limit & 0xFFFF;
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access      = access;
                     }

                     void gdt_init(void) {
                         gdtp.limit = sizeof(gdt_entry_t) * 3 - 1;
                         gdtp.base  = (uint32_t)&gdt;

                         /* 0: null descriptor */
                         set_gate(0, 0, 0, 0, 0);

                         /* 1: kernel code, base 0, limit 4 ГБ, ring 0, executable, readable */
                         set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF);

                         /* 2: kernel data, base 0, limit 4 ГБ, ring 0, writable */
                         set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);

                         gdt_load((uint32_t)&gdtp);
                     }
