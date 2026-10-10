#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

void vga_init_fb(
    uint32_t addr,
    uint32_t pitch,
    uint32_t width,
    uint32_t height,
    uint8_t bpp
);

void vga_init(void);

void vga_clear(void);
void vga_putc(char c);
void vga_print(const char *s);
void vga_print_dec(uint32_t n);
void vga_print_hex(uint32_t n);
void vga_write(
    const void *data,
    uint32_t size
);

void vga_set_fg(uint32_t rgb);
void vga_set_bg(uint32_t rgb);

uint32_t vga_cols(void);
uint32_t vga_rows(void);

uint32_t vga_width(void);
uint32_t vga_height(void);

int vga_is_framebuffer(void);

void vga_draw_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color
);

void vga_draw_rect(
    uint32_t x,
    uint32_t y,
    uint32_t w,
    uint32_t h,
    uint32_t color
);

void vga_set_cursor_pixel(
    uint32_t x,
    uint32_t y
);

#endif
