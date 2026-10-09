#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_print(const char *s);
void vga_print_dec(uint32_t n);
void vga_print_hex(uint32_t n);
void vga_write(const void *data, uint32_t size);

#endif
