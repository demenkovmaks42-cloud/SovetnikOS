
#include "vga.h"
#include "types.h"

#define VGA_TEXT_MEMORY 0xB8000u

#define FONT_W 8u
#define FONT_H 16u

#define DEFAULT_FG 0x00FFFFFFu
#define DEFAULT_BG 0x00000000u

#define TEXT_COLS 80u
#define TEXT_ROWS 25u

#define MIN_U32(a, b) ((a) < (b) ? (a) : (b))

/*
 * sovetnikOS VGA / Framebuffer driver
 *
 * Поддерживает:
 *   - VGA text mode 80x25;
 *   - framebuffer 16, 24 и 32 bpp;
 *   - вывод текста и чисел;
 *   - рисование пикселей и прямоугольников;
 *   - управление текстовым курсором.
 */

static volatile uint16_t *text_memory =
(volatile uint16_t *)VGA_TEXT_MEMORY;

static volatile uint8_t *fb = NULL;

static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t fb_bpp = 0;

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;

static uint32_t text_col = 0;
static uint32_t text_row = 0;

static uint32_t fg_color = DEFAULT_FG;
static uint32_t bg_color = DEFAULT_BG;

static uint8_t text_color = 0x0F;

/* =========================================================
 * Встроенный шрифт 5x7 для букв и цифр.
 * Каждый символ занимает 5 столбцов и 7 строк.
 * ========================================================= */

static const uint8_t font5x7[][5] = {
    /* A-Z */
    {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43},

    /* 0-9 */
    {0x3E,0x51,0x49,0x45,0x3E},
    {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},
    {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1E}
};

static const uint8_t *get_glyph(char c)
{
    static const uint8_t blank[5] = {0,0,0,0,0};

    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');

    if (c >= 'A' && c <= 'Z')
        return font5x7[(uint32_t)(c - 'A')];

    if (c >= '0' && c <= '9')
        return font5x7[26u + (uint32_t)(c - '0')];

    return blank;
}

/* =========================================================
 * Цвета и framebuffer
 * ========================================================= */

static uint32_t rgb_to_565(uint32_t color)
{
    uint32_t r = (color >> 16) & 0xFF;
    uint32_t g = (color >> 8) & 0xFF;
    uint32_t b = color & 0xFF;

    return ((r >> 3) << 11) |
    ((g >> 2) << 5) |
    (b >> 3);
}

static void put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (fb == NULL)
        return;

    if (x >= fb_width || y >= fb_height)
        return;

    if (fb_bpp != 16 && fb_bpp != 24 && fb_bpp != 32)
        return;

    uint32_t bytes_per_pixel = (uint32_t)fb_bpp / 8u;
    uint32_t offset = y * fb_pitch + x * bytes_per_pixel;

    if (fb_bpp == 32) {
        volatile uint32_t *p =
        (volatile uint32_t *)(fb + offset);

        *p = color & 0x00FFFFFFu;
    } else if (fb_bpp == 24) {
        fb[offset]     = (uint8_t)(color & 0xFF);
        fb[offset + 1] = (uint8_t)((color >> 8) & 0xFF);
        fb[offset + 2] = (uint8_t)((color >> 16) & 0xFF);
    } else {
        volatile uint16_t *p =
        (volatile uint16_t *)(fb + offset);

        *p = (uint16_t)rgb_to_565(color);
    }
}

static void fill_rect(
    uint32_t x,
    uint32_t y,
    uint32_t w,
    uint32_t h,
    uint32_t color
)
{
    if (fb == NULL || w == 0 || h == 0)
        return;

    if (x >= fb_width || y >= fb_height)
        return;

    if (w > fb_width - x)
        w = fb_width - x;

    if (h > fb_height - y)
        h = fb_height - y;

    for (uint32_t py = y; py < y + h; py++) {
        for (uint32_t px = x; px < x + w; px++)
            put_pixel(px, py, color);
    }
}

/* =========================================================
 * Рисование символов
 * ========================================================= */

static void draw_char(char c, uint32_t x, uint32_t y)
{
    const uint8_t *glyph = get_glyph(c);

    for (uint32_t row = 0; row < 7; row++) {
        for (uint32_t col = 0; col < 5; col++) {
            uint32_t color =
            (glyph[col] & (1u << row))
            ? fg_color
            : bg_color;

            put_pixel(x + col + 1, y + row + 4, color);
        }
    }
}

static void scroll_up(void)
{
    if (fb != NULL) {
        if (fb_height <= FONT_H) {
            fill_rect(0, 0, fb_width, fb_height, bg_color);
            cursor_y = 0;
            return;
        }

        for (uint32_t y = 0; y < fb_height - FONT_H; y++) {
            for (uint32_t x = 0; x < fb_width; x++) {
                uint32_t src_y = y + FONT_H;
                uint32_t bpp = (uint32_t)fb_bpp / 8u;

                if (fb_bpp != 16 && fb_bpp != 24 &&
                    fb_bpp != 32)
                    return;

                uint32_t src = src_y * fb_pitch + x * bpp;
                uint32_t dst = y * fb_pitch + x * bpp;

                for (uint32_t byte = 0; byte < bpp; byte++)
                    fb[dst + byte] = fb[src + byte];
            }
        }

        fill_rect(
            0,
            fb_height - FONT_H,
            fb_width,
            FONT_H,
            bg_color
        );

        cursor_y = fb_height - FONT_H;
        return;
    }

    for (uint32_t row = 1; row < TEXT_ROWS; row++) {
        for (uint32_t col = 0; col < TEXT_COLS; col++) {
            text_memory[(row - 1) * TEXT_COLS + col] =
            text_memory[row * TEXT_COLS + col];
        }
    }

    for (uint32_t col = 0; col < TEXT_COLS; col++) {
        text_memory[(TEXT_ROWS - 1) * TEXT_COLS + col] =
        (uint16_t)text_color << 8 | ' ';
    }

    text_row = TEXT_ROWS - 1;
}

/* =========================================================
 * Инициализация
 * ========================================================= */

void vga_init_fb(
    uint32_t addr,
    uint32_t pitch,
    uint32_t width,
    uint32_t height,
    uint8_t bpp
)
{
    fb = NULL;
    fb_pitch = 0;
    fb_width = 0;
    fb_height = 0;
    fb_bpp = 0;

    if (addr == 0 || width == 0 || height == 0)
        return;

    if (bpp != 16 && bpp != 24 && bpp != 32)
        return;

    uint32_t bytes_per_pixel = (uint32_t)bpp / 8u;

    if (width > 0xFFFFFFFFu / bytes_per_pixel)
        return;

    if (pitch < width * bytes_per_pixel)
        return;

    fb = (volatile uint8_t *)addr;
    fb_pitch = pitch;
    fb_width = width;
    fb_height = height;
    fb_bpp = bpp;

    cursor_x = 0;
    cursor_y = 0;
}

void vga_init(void)
{
    fb = NULL;
    fb_pitch = 0;
    fb_width = 0;
    fb_height = 0;
    fb_bpp = 0;

    text_memory = (volatile uint16_t *)VGA_TEXT_MEMORY;

    text_col = 0;
    text_row = 0;

    cursor_x = 0;
    cursor_y = 0;

    fg_color = DEFAULT_FG;
    bg_color = DEFAULT_BG;
    text_color = 0x0F;

    vga_clear();
}

/* =========================================================
 * Очистка экрана
 * ========================================================= */

void vga_clear(void)
{
    if (fb != NULL) {
        fill_rect(0, 0, fb_width, fb_height, bg_color);
        cursor_x = 0;
        cursor_y = 0;
        return;
    }

    uint16_t blank = ((uint16_t)text_color << 8) | ' ';

    for (uint32_t i = 0; i < TEXT_COLS * TEXT_ROWS; i++)
        text_memory[i] = blank;

    text_col = 0;
    text_row = 0;
}

/* =========================================================
 * Цвет текста
 * ========================================================= */

void vga_set_fg(uint32_t rgb)
{
    fg_color = rgb & 0x00FFFFFFu;

    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8) & 0xFF;
    uint32_t b = rgb & 0xFF;

    uint32_t intensity =
    (r * 30u + g * 59u + b * 11u) / 100u;

    if (intensity > 170)
        text_color = (text_color & 0xF0u) | 0x0Fu;
    else if (intensity > 80)
        text_color = (text_color & 0xF0u) | 0x07u;
    else
        text_color = (text_color & 0xF0u) | 0x08u;
}

void vga_set_bg(uint32_t rgb)
{
    bg_color = rgb & 0x00FFFFFFu;

    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8) & 0xFF;
    uint32_t b = rgb & 0xFF;

    uint32_t intensity =
    (r * 30u + g * 59u + b * 11u) / 100u;

    if (intensity > 170)
        text_color = (text_color & 0x0Fu) | 0xF0u;
    else if (intensity > 80)
        text_color = (text_color & 0x0Fu) | 0x70u;
    else
        text_color = (text_color & 0x0Fu) | 0x80u;
}

/* =========================================================
 * Размеры экрана
 * ========================================================= */

uint32_t vga_cols(void)
{
    if (fb != NULL)
        return fb_width / FONT_W;

    return TEXT_COLS;
}

uint32_t vga_rows(void)
{
    if (fb != NULL)
        return fb_height / FONT_H;

    return TEXT_ROWS;
}

uint32_t vga_width(void)
{
    if (fb != NULL)
        return fb_width;

    return TEXT_COLS * FONT_W;
}

uint32_t vga_height(void)
{
    if (fb != NULL)
        return fb_height;

    return TEXT_ROWS * FONT_H;
}

int vga_is_framebuffer(void)
{
    return fb != NULL &&
    fb_width != 0 &&
    fb_height != 0;
}

/* =========================================================
 * Вывод символов
 * ========================================================= */

void vga_putc(char c)
{
    if (fb != NULL) {
        if (c == '\n') {
            cursor_x = 0;
            cursor_y += FONT_H;

            if (cursor_y + FONT_H > fb_height)
                scroll_up();

            return;
        }

        if (c == '\r') {
            cursor_x = 0;
            return;
        }

        if (c == '\b') {
            if (cursor_x >= FONT_W)
                cursor_x -= FONT_W;

            fill_rect(
                cursor_x,
                cursor_y,
                FONT_W,
                FONT_H,
                bg_color
            );

            return;
        }

        if (cursor_x + FONT_W > fb_width) {
            cursor_x = 0;
            cursor_y += FONT_H;
        }

        if (cursor_y + FONT_H > fb_height)
            scroll_up();

        draw_char(c, cursor_x, cursor_y);
        cursor_x += FONT_W;

        return;
    }

    if (c == '\n') {
        text_col = 0;
        text_row++;
    } else if (c == '\r') {
        text_col = 0;
    } else if (c == '\b') {
        if (text_col > 0)
            text_col--;

        text_memory[text_row * TEXT_COLS + text_col] =
        ((uint16_t)text_color << 8) | ' ';

        return;
    } else {
        if (text_col >= TEXT_COLS) {
            text_col = 0;
            text_row++;
        }

        if (text_row < TEXT_ROWS) {
            text_memory[text_row * TEXT_COLS + text_col] =
            ((uint16_t)text_color << 8) |
            (uint8_t)c;
        }

        text_col++;
    }

    if (text_row >= TEXT_ROWS)
        scroll_up();
}

/* =========================================================
 * Вывод строк
 * ========================================================= */

void vga_print(const char *s)
{
    if (s == NULL)
        return;

    while (*s != '\0') {
        vga_putc(*s);
        s++;
    }
}

/* =========================================================
 * Вывод десятичного числа
 * ========================================================= */

void vga_print_dec(uint32_t n)
{
    char buffer[11];
    uint32_t i = 0;

    if (n == 0) {
        vga_putc('0');
        return;
    }

    while (n > 0 && i < sizeof(buffer)) {
        buffer[i++] = (char)('0' + n % 10u);
        n /= 10u;
    }

    while (i > 0)
        vga_putc(buffer[--i]);
}

/* =========================================================
 * Вывод шестнадцатеричного числа
 * ========================================================= */

void vga_print_hex(uint32_t n)
{
    static const char digits[] = "0123456789ABCDEF";

    vga_print("0x");

    int started = 0;

    for (int shift = 28; shift >= 0; shift -= 4) {
        uint32_t digit = (n >> shift) & 0xFu;

        if (digit != 0 || started || shift == 0) {
            vga_putc(digits[digit]);
            started = 1;
        }
    }
}

/* =========================================================
 * Вывод блока байтов
 * ========================================================= */

void vga_write(const void *data, uint32_t size)
{
    if (data == NULL)
        return;

    const uint8_t *bytes = (const uint8_t *)data;

    for (uint32_t i = 0; i < size; i++)
        vga_putc((char)bytes[i]);
}

/* =========================================================
 * Графические функции для GUI и мыши
 * ========================================================= */

void vga_draw_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    put_pixel(x, y, color & 0x00FFFFFFu);
}

void vga_draw_rect(
    uint32_t x,
    uint32_t y,
    uint32_t w,
    uint32_t h,
    uint32_t color
)
{
    fill_rect(x, y, w, h, color & 0x00FFFFFFu);
}

void vga_set_cursor_pixel(uint32_t x, uint32_t y)
{
    if (fb == NULL)
        return;

    if (x >= fb_width)
        x = fb_width - 1;

    if (y >= fb_height)
        y = fb_height - 1;

    cursor_x = x;
    cursor_y = y;
}
