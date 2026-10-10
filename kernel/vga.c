#include "vga.h"
#include "font8x16.h"

#define FONT_W 8
#define FONT_H 16

#define VGA_BUFFER \
((volatile uint16_t *)0xB8000)

static volatile uint8_t *fb = NULL;

static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;

static uint8_t fb_bpp = 0;

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;

static uint32_t fg_color =
0x00C0C0C0;

static uint32_t bg_color =
0x00000000;

static int text_row = 0;
static int text_col = 0;

static uint8_t text_color = 0x07;

static void put_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color
)
{
    if (!fb)
        return;

    if (x >= fb_width ||
        y >= fb_height)
        return;

    uint32_t bytes =
    fb_bpp / 8;

    uint32_t offset =
    y * fb_pitch +
    x * bytes;

    if (fb_bpp == 32) {

        *(volatile uint32_t *)
        (fb + offset) = color;

    } else if (fb_bpp == 24) {

        fb[offset + 0] =
        (uint8_t)(color & 0xFF);

        fb[offset + 1] =
        (uint8_t)((color >> 8) & 0xFF);

        fb[offset + 2] =
        (uint8_t)((color >> 16) & 0xFF);

    } else if (fb_bpp == 16) {

        uint16_t c =
        (uint16_t)(
            (((color >> 19) & 0x1F) << 11) |
            (((color >> 10) & 0x3F) << 5) |
            (((color >> 3) & 0x1F))
        );

        *(volatile uint16_t *)
        (fb + offset) = c;
    }
}

void vga_draw_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color
)
{
    put_pixel(x, y, color);
}

void vga_draw_rect(
    uint32_t x,
    uint32_t y,
    uint32_t w,
    uint32_t h,
    uint32_t color
)
{
    if (!fb)
        return;

    if (x >= fb_width ||
        y >= fb_height)
        return;

    if (w > fb_width - x)
        w = fb_width - x;

    if (h > fb_height - y)
        h = fb_height - y;

    for (uint32_t yy = 0;
         yy < h;
    yy++) {

        for (uint32_t xx = 0;
             xx < w;
        xx++) {

            put_pixel(
                x + xx,
                y + yy,
                color
            );
        }
    }
}

static void scroll_up(void)
{
    if (!fb ||
        fb_height <= FONT_H)
        return;

    uint32_t bytes =
    fb_pitch *
    (fb_height - FONT_H);

    uint8_t *dst =
    (uint8_t *)fb;

    uint8_t *src =
    (uint8_t *)fb +
    fb_pitch * FONT_H;

    for (uint32_t i = 0;
         i < bytes;
    i++)
         dst[i] = src[i];

         vga_draw_rect(
             0,
             fb_height - FONT_H,
             fb_width,
             FONT_H,
             bg_color
         );
}

void vga_init_fb(
    uint32_t addr,
    uint32_t pitch,
    uint32_t width,
    uint32_t height,
    uint8_t bpp
)
{
    if ((bpp != 32 &&
        bpp != 24 &&
        bpp != 16) ||
        width == 0 ||
        height == 0) {

        vga_init();
    return;
        }

        fb =
        (volatile uint8_t *)addr;

        fb_pitch = pitch;
        fb_width = width;
        fb_height = height;
        fb_bpp = bpp;

        cursor_x = 0;
        cursor_y = 0;

        vga_draw_rect(
            0,
            0,
            width,
            height,
            bg_color
        );
}

void vga_init(void)
{
    fb = NULL;

    for (int i = 0;
         i < VGA_WIDTH * VGA_HEIGHT;
    i++) {

        VGA_BUFFER[i] =
        ((uint16_t)text_color << 8) |
        ' ';
    }

    text_row = 0;
    text_col = 0;
}

void vga_set_fg(uint32_t rgb)
{
    fg_color = rgb;
}

void vga_set_bg(uint32_t rgb)
{
    bg_color = rgb;
}

uint32_t vga_cols(void)
{
    if (fb)
        return fb_width / FONT_W;

    return VGA_WIDTH;
}

uint32_t vga_rows(void)
{
    if (fb)
        return fb_height / FONT_H;

    return VGA_HEIGHT;
}

uint32_t vga_width(void)
{
    if (fb)
        return fb_width;

    return VGA_WIDTH * FONT_W;
}

uint32_t vga_height(void)
{
    if (fb)
        return fb_height;

    return VGA_HEIGHT * FONT_H;
}

int vga_is_framebuffer(void)
{
    return fb != NULL;
}

void vga_set_cursor_pixel(
    uint32_t x,
    uint32_t y
)
{
    if (fb) {

        if (x < fb_width)
            cursor_x = x;
        else
            cursor_x =
            fb_width - FONT_W;

        if (y < fb_height)
            cursor_y = y;
        else
            cursor_y =
            fb_height - FONT_H;

    } else {

        text_col =
        (int)(x / FONT_W);

        text_row =
        (int)(y / FONT_H);

        if (text_col >= VGA_WIDTH)
            text_col = VGA_WIDTH - 1;

        if (text_row >= VGA_HEIGHT)
            text_row = VGA_HEIGHT - 1;
    }
}

void vga_clear(void)
{
    if (fb) {

        vga_draw_rect(
            0,
            0,
            fb_width,
            fb_height,
            bg_color
        );

        cursor_x = 0;
        cursor_y = 0;

        return;
    }

    for (int i = 0;
         i < VGA_WIDTH * VGA_HEIGHT;
    i++) {

        VGA_BUFFER[i] =
        ((uint16_t)text_color << 8) |
        ' ';
    }

    text_row = 0;
    text_col = 0;
}

void vga_putc(char c)
{
    if (fb) {

        if (c == '\b') {

            if (cursor_x >= FONT_W) {
                cursor_x -= FONT_W;

                vga_draw_rect(
                    cursor_x,
                    cursor_y,
                    FONT_W,
                    FONT_H,
                    bg_color
                );
            }

            return;
        }

        if (c == '\n') {

            cursor_x = 0;
            cursor_y += FONT_H;

        } else if (c == '\r') {

            cursor_x = 0;

        } else if (c == '\t') {

            cursor_x =
            (cursor_x +
            FONT_W * 4) &
            ~(FONT_W * 4 - 1);

        } else {

            uint8_t ch =
            (uint8_t)c;

            if (ch < 32 ||
                ch > 126)
                ch = '?';

            const uint8_t *glyph =
            font8x16[ch - 32];

            for (int r = 0;
                 r < FONT_H;
            r++) {

                uint8_t bits =
                glyph[r];

                for (int x = 0;
                     x < FONT_W;
                x++) {

                    put_pixel(
                        cursor_x + x,
                        cursor_y + r,
                        (bits & (0x80 >> x)) ?
                        fg_color :
                        bg_color
                    );
                }
            }

            cursor_x += FONT_W;
        }

        if (cursor_x + FONT_W >
            fb_width) {

            cursor_x = 0;
        cursor_y += FONT_H;
            }

            if (cursor_y + FONT_H >
                fb_height) {

                scroll_up();

            cursor_y =
            fb_height - FONT_H;
                }

                return;
    }

    if (c == '\n') {

        text_col = 0;
        text_row++;

    } else if (c == '\r') {

        text_col = 0;

    } else if (c == '\t') {

        text_col =
        (text_col + 4) & ~3;

    } else if (c == '\b') {

        if (text_col > 0)
            text_col--;

    } else {

        VGA_BUFFER[
            text_row * VGA_WIDTH +
            text_col
        ] =
        ((uint16_t)text_color << 8) |
        (uint8_t)c;

        text_col++;

        if (text_col >= VGA_WIDTH) {
            text_col = 0;
            text_row++;
        }
    }

    if (text_row >= VGA_HEIGHT) {

        for (int y = 0;
             y < VGA_HEIGHT - 1;
        y++) {

            for (int x = 0;
                 x < VGA_WIDTH;
            x++) {

                VGA_BUFFER[
                    y * VGA_WIDTH + x
                ] =
                VGA_BUFFER[
                    (y + 1) *
                    VGA_WIDTH + x
                ];
            }
        }

        for (int x = 0;
             x < VGA_WIDTH;
        x++) {

            VGA_BUFFER[
                (VGA_HEIGHT - 1) *
                VGA_WIDTH + x
            ] =
            ((uint16_t)text_color << 8) |
            ' ';
        }

        text_row =
        VGA_HEIGHT - 1;
    }
}

void vga_print(const char *s)
{
    while (*s)
        vga_putc(*s++);
}

void vga_print_dec(uint32_t n)
{
    char buf[11];
    int i = 0;

    if (!n) {
        vga_putc('0');
        return;
    }

    while (n && i < 10) {
        buf[i++] =
        (char)('0' + n % 10);

        n /= 10;
    }

    while (i)
        vga_putc(buf[--i]);
}

void vga_print_hex(uint32_t n)
{
    const char *hex =
    "0123456789ABCDEF";

    vga_print("0x");

    for (int i = 28;
         i >= 0;
    i -= 4) {

        vga_putc(
            hex[(n >> i) & 0xF]
        );
    }
}

void vga_write(
    const void *data,
    uint32_t size
)
{
    const char *p =
    (const char *)data;

    for (uint32_t i = 0;
         i < size;
    i++)
         vga_putc(p[i]);
}
