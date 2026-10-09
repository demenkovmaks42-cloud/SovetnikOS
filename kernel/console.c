#include "../include/console.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static volatile unsigned short *vga = (volatile unsigned short*)0xB8000;
static int row = 0;
static int col = 0;
static uint8_t color = 0x07;

static unsigned short cell(char c)
{
    return (unsigned short)c | ((unsigned short)color << 8);
}

void console_set_color(uint8_t c)
{
    color = c;
}

void console_clear(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = cell(' ');

    row = 0;
    col = 0;
}

void console_putc(char c)
{
    if (c == '\n') {
        col = 0;
        row++;
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

    if (row >= VGA_HEIGHT) {
        for (int y = 1; y < VGA_HEIGHT; y++)
            for (int x = 0; x < VGA_WIDTH; x++)
                vga[(y - 1) * VGA_WIDTH + x] = vga[y * VGA_WIDTH + x];

        for (int x = 0; x < VGA_WIDTH; x++)
            vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = cell(' ');

        row = VGA_HEIGHT - 1;
    }
}

void console_puts(const char *s)
{
    while (*s)
        console_putc(*s++);
}

void console_prompt(void)
{
    console_puts("sovetnikOS> ");
}
