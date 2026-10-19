
#include <stdint.h>
#include "vga.h"

void exception_handler(uint32_t *stack)
{
    uint32_t error_code = stack[13];
    uint32_t eip        = stack[14];
    uint32_t cs         = stack[15];
    uint32_t eflags     = stack[16];

    vga_set_fg(15);
    vga_set_bg(4);
    vga_clear();

    vga_print("FATAL: CPU EXCEPTION\n");
    vga_print("The kernel cannot continue safely.\n\n");

    vga_print("EIP: ");
    vga_print_hex(eip);
    vga_print("\nCS: ");
    vga_print_hex(cs);
    vga_print("\nEFLAGS: ");
    vga_print_hex(eflags);
    vga_print("\nError code: ");
    vga_print_hex(error_code);
    vga_print("\n\nSystem halted.\n");

    __asm__ volatile ("cli");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
