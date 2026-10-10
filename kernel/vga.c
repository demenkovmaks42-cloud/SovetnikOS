#include "vga.h"
#include "font8x16.h"

#define FONT_W 8
#define FONT_H 16

/* --- Framebuffer --- */
static volatile uint32_t *fb   = NULL;
static uint32_t fb_pitch   = 0;
static uint32_t fb_width   = 0;
static uint32_t fb_height  = 0;
static uint8_t  fb_bpp     = 0;

static uint32_t cursor_x = 0;   /* в пикселях */
static uint32_t cursor_y = 0;

static uint32_t fg_color = 0x00C0C0C0;  /* светло-серый */
static uint32_t bg_color = 0x00000000;  /* чёрный */

/* --- VGA text fallback --- */
#define VGA_BUFFER ((volatile uint16_t*)0xB8000)
static int text_row = 0;
static int text_col = 0;
static uint8_t text_color = 0x07;

/* ================== Вспомогательные ================== */

static void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb || x >= fb_width || y >= fb_height) return;
    uint32_t offset = y * fb_pitch + x * (fb_bpp / 8);
    if (fb_bpp == 32) {
        fb[offset / 4] = color;
    } else if (fb_bpp == 24) {
        uint8_t *p = (uint8_t*)fb + offset;
        p[0] = color & 0xFF;
        p[1] = (color >> 8) & 0xFF;
        p[2] = (color >> 16) & 0xFF;
    } else if (fb_bpp == 16) {
        uint16_t *p = (uint16_t*)((uint8_t*)fb + offset);
        uint16_t c = ((color >> 8) & 0xF800) |
        ((color >> 5) & 0x07E0) |
        ((color >> 3) & 0x001F);
        *p = c;
    }
}

static void fill_rect(uint32_t x, uint32_t y,
                      uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t yy = 0; yy < h; yy++)
        for (uint32_t xx = 0; xx < w; xx++)
            put_pixel(x + xx, y + yy, color);
                      }

                      static void scroll_up(void) {
                          uint32_t line_bytes = fb_pitch * FONT_H;
                          uint32_t copy_bytes = fb_pitch * (fb_height - FONT_H);
                          uint8_t *dst = (uint8_t*)fb;
                          uint8_t *src = (uint8_t*)fb + line_bytes;
                          for (uint32_t i = 0; i < copy_bytes; i++) dst[i] = src[i];
                          fill_rect(0, fb_height - FONT_H, fb_width, FONT_H, bg_color);
                      }

                      /* ================== Инициализация ================== */

                      void vga_init_fb(uint32_t addr, uint32_t pitch,
                                       uint32_t width, uint32_t height, uint8_t bpp) {
                          fb        = (volatile uint32_t*)addr;
                          fb_pitch  = pitch;
                          fb_width  = width;
                          fb_height = height;
                          fb_bpp    = bpp;
                          cursor_x  = 0;
                          cursor_y  = 0;
                          fill_rect(0, 0, width, height, bg_color);
                                       }

                                       void vga_init(void) {
                                           for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
                                               VGA_BUFFER[i] = (text_color << 8) | ' ';
                                           text_row = 0;
                                           text_col = 0;
                                       }

                                       void vga_set_fg(uint32_t rgb) { fg_color = rgb; }
                                       void vga_set_bg(uint32_t rgb) { bg_color = rgb; }

                                       uint32_t vga_cols(void) {
                                           if (fb) return fb_width / FONT_W;
                                           return VGA_WIDTH;
                                       }

                                       uint32_t vga_rows(void) {
                                           if (fb) return fb_height / FONT_H;
                                           return VGA_HEIGHT;
                                       }

                                       /* ================== Вывод ================== */

                                       void vga_clear(void) {
                                           if (fb) {
                                               fill_rect(0, 0, fb_width, fb_height, bg_color);
                                               cursor_x = 0;
                                               cursor_y = 0;
                                               return;
                                           }
                                           for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
                                               VGA_BUFFER[i] = (text_color << 8) | ' ';
                                           text_row = 0;
                                           text_col = 0;
                                       }

                                       void vga_putc(char c) {
                                           /* framebuffer path */
                                           if (fb) {
                                               if (c == '\b') {
                                                   if (cursor_x >= FONT_W) {
                                                       cursor_x -= FONT_W;
                                                       fill_rect(cursor_x, cursor_y, FONT_W, FONT_H, bg_color);
                                                   }
                                                   return;
                                               }
                                               if (c == '\n') {
                                                   cursor_x = 0;
                                                   cursor_y += FONT_H;
                                                   if (cursor_y + FONT_H > fb_height) {
                                                       scroll_up();
                                                       cursor_y = fb_height - FONT_H;
                                                   }
                                                   return;
                                               }
                                               if (c == '\r') { cursor_x = 0; return; }
                                               if (c == '\t') {
                                                   cursor_x = (cursor_x + FONT_W * 4) & ~(FONT_W * 4 - 1);
                                                   if (cursor_x >= fb_width) cursor_x = fb_width - FONT_W;
                                                   return;
                                               }
                                               uint8_t ch = (uint8_t)c;
                                               if (ch < 32 || ch > 126) ch = '?';
                                               const uint8_t *glyph = font8x16[ch - 32];
                                               for (int row = 0; row < FONT_H; row++) {
                                                   uint8_t bits = glyph[row];
                                                   for (int col = 0; col < FONT_W; col++) {
                                                       uint32_t color = (bits & (0x80 >> col)) ? fg_color : bg_color;
                                                       put_pixel(cursor_x + col, cursor_y + row, color);
                                                   }
                                               }
                                               cursor_x += FONT_W;
                                               if (cursor_x + FONT_W > fb_width) {
                                                   cursor_x = 0;
                                                   cursor_y += FONT_H;
                                                   if (cursor_y + FONT_H > fb_height) {
                                                       scroll_up();
                                                       cursor_y = fb_height - FONT_H;
                                                   }
                                               }
                                               return;
                                           }

                                           /* fallback: VGA text */
                                           if (c == '\n') { text_col = 0; text_row++; }
                                           else if (c == '\r') { text_col = 0; }
                                           else if (c == '\t') { text_col = (text_col + 4) & ~3; }
                                           else if (c == '\b') { if (text_col > 0) text_col--; }
                                           else {
                                               VGA_BUFFER[text_row * VGA_WIDTH + text_col] =
                                               (text_color << 8) | (uint8_t)c;
                                               text_col++;
                                               if (text_col >= VGA_WIDTH) { text_col = 0; text_row++; }
                                           }
                                           if (text_row >= VGA_HEIGHT) {
                                               for (int y = 0; y < VGA_HEIGHT - 1; y++)
                                                   for (int x = 0; x < VGA_WIDTH; x++)
                                                       VGA_BUFFER[y * VGA_WIDTH + x] =
                                                       VGA_BUFFER[(y + 1) * VGA_WIDTH + x];
                                                   for (int x = 0; x < VGA_WIDTH; x++)
                                                       VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
                                                       (text_color << 8) | ' ';
                                                   text_row = VGA_HEIGHT - 1;
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
                                           for (int i = 28; i >= 0; i -= 4) vga_putc(hex[(n >> i) & 0xF]);
                                       }

                                       void vga_write(const void *data, uint32_t size) {
                                           const char *p = (const char*)data;
                                           for (uint32_t i = 0; i < size; i++) vga_putc(p[i]);
                                       }
