#include "vga.h"

#define VGA_BUFFER ((volatile uint16_t*)0xB8000)

static int row = 0;
static int col = 0;
static uint8_t color = 0x07; // серый на чёрном

static void scroll(void) {
    if (row < VGA_HEIGHT) return;
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[y * VGA_WIDTH + x] = VGA_BUFFER[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (color << 8) | ' ';
    }
    row = VGA_HEIGHT - 1;
}

void vga_init(void) {
    row = 0;
    col = 0;
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = (color << 8) | ' ';
    }
    row = 0;
    col = 0;
}

void vga_putc(char c) {
    if (c == '\n') {
        col = 0;
        row++;
        scroll();
        return;
    }
    if (c == '\r') {
        col = 0;
        return;
    }
    if (c == '\t') {
        col = (col + 4) & ~3;
        return;
    }
    VGA_BUFFER[row * VGA_WIDTH + col] = (color << 8) | (uint8_t)c;
    col++;
    if (col >= VGA_WIDTH) {
        col = 0;
        row++;
        scroll();
    }
}

void vga_print(const char *s) {
    while (*s) vga_putc(*s++);
}

void vga_print_dec(uint32_t n) {
    char buf[12];
    int i = 0;
    if (n == 0) { vga_putc('0'); return; }
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i--) vga_putc(buf[i]);
}

void vga_print_hex(uint32_t n) {
    const char *hex = "0123456789ABCDEF";
    vga_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        vga_putc(hex[(n >> i) & 0xF]);
    }
}

void vga_write(const void *data, uint32_t size) {
    const char *p = (const char*)data;
    for (uint32_t i = 0; i < size; i++) vga_putc(p[i]);
}
