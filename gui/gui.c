#include "../include/gui.h"

/*
 * sovetnikOS 0.1 uses the VGA text console as its guaranteed display.
 * The GUI drawing routines are retained as a foundation for the next
 * framebuffer/VBE version.
 */

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
static volatile unsigned short *vga = (volatile unsigned short*)0xB8000;

static unsigned int row = 0;
static unsigned int col = 0;
static uint8_t attr = 0x07;

static unsigned short cell(char c)
{
    return (unsigned short)c | ((unsigned short)attr << 8);
}

static void putc_internal(char c)
{
    if (c == '\n') {
        col = 0;
        row++;
    } else if (c == '\r') {
        col = 0;
    } else if (c == '\b') {
        if (col > 0) {
            col--;
            vga[row * VGA_WIDTH + col] = cell(' ');
        }
    } else {
        vga[row * VGA_WIDTH + col] = cell(c);
        col++;
        if (col >= VGA_WIDTH) {
            col = 0;
            row++;
        }
    }

    if (row >= VGA_HEIGHT)
        row = 0;
}

void gui_clear(uint8_t color)
{
    attr = (uint8_t)((color & 0x0F) | 0x00);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = cell(' ');

    row = 0;
    col = 0;
}

void gui_pixel(int x, int y, uint8_t color)
{
    (void)x; (void)y; (void)color;
}

void gui_rect(int x, int y, int w, int h, uint8_t color)
{
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

void gui_frame(int x, int y, int w, int h, uint8_t color)
{
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

void gui_cursor(int x, int y)
{
    (void)x; (void)y;
}

static void print(const char *s)
{
    while (*s)
        putc_internal(*s++);
}

void gui_init(void)
{
    gui_clear(0x07);

    attr = 0x1F;
    print("========================================\n");
    print("          sovetnikOS 0.1\n");
    print("========================================\n\n");

    attr = 0x07;
    print("Minimal x86 operating system\n");
    print("NASM + C + GRUB + QEMU\n\n");

    print("Hardware:\n");
    print("  Keyboard: PS/2\n");
    print("  Mouse:    PS/2\n");
    print("  Display:  VGA text mode\n\n");

    print("Type 'help' for commands.\n\n");
}

void gui_puts(const char *s)
{
    print(s);
}

void gui_set_attribute(uint8_t a)
{
    attr = a;
}
