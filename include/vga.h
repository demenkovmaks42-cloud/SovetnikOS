#ifndef VGA_H
#define VGA_H

#include "types.h"

/* Старый VGA text (80x25) — fallback, если framebuffer не пришёл */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* Инициализация framebuffer из Multiboot */
void vga_init_fb(uint32_t addr, uint32_t pitch,
                 uint32_t width, uint32_t height, uint8_t bpp);

/* Инициализация в текстовом режиме (fallback) */
void vga_init(void);

void vga_clear(void);
void vga_putc(char c);
void vga_print(const char *s);
void vga_print_dec(uint32_t n);
void vga_print_hex(uint32_t n);
void vga_write(const void *data, uint32_t size);

/* Цвета для framebuffer */
void vga_set_fg(uint32_t rgb);
void vga_set_bg(uint32_t rgb);

/* Размеры экрана (символы) для framebuffer */
uint32_t vga_cols(void);
uint32_t vga_rows(void);

#endif
