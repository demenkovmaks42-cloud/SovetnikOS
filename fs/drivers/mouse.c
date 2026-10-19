#include "mouse.h"
#include "keyboard.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

#define STATUS_OUTPUT_FULL 0x01
#define STATUS_INPUT_FULL  0x02
#define STATUS_AUX_DATA    0x20

#define MOUSE_LEFT   0x01
#define MOUSE_RIGHT  0x02
#define MOUSE_MIDDLE 0x04

static volatile int mx = 160;
static volatile int my = 100;
static volatile uint8_t buttons = 0;

static uint32_t mouse_width = 320;
static uint32_t mouse_height = 200;

static uint8_t packet[3];
static uint8_t packet_pos = 0;
static int present = 0;

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static int wait_input_clear(void)
{
    for (uint32_t i = 0; i < 100000u; ++i) {
        if (!(inb(PS2_STATUS_PORT) & STATUS_INPUT_FULL))
            return 1;
    }

    return 0;
}

static int wait_output(void)
{
    for (uint32_t i = 0; i < 100000u; ++i) {
        if (inb(PS2_STATUS_PORT) & STATUS_OUTPUT_FULL)
            return 1;
    }

    return 0;
}

static int controller_command(uint8_t command)
{
    if (!wait_input_clear())
        return 0;

    outb(PS2_COMMAND_PORT, command);
    return 1;
}

static int mouse_write(uint8_t value)
{
    if (!controller_command(0xD4))
        return 0;

    if (!wait_input_clear())
        return 0;

    outb(PS2_DATA_PORT, value);
    return 1;
}

static int mouse_command(uint8_t command)
{
    uint8_t response;

    if (!mouse_write(command))
        return 0;

    for (uint32_t i = 0; i < 100000u; ++i) {
        if (!(inb(PS2_STATUS_PORT) & STATUS_OUTPUT_FULL))
            continue;

        uint8_t status = inb(PS2_STATUS_PORT);
        uint8_t value = inb(PS2_DATA_PORT);

        if (status & STATUS_AUX_DATA) {
            response = value;
            return response == 0xFA;
        }

        /* Не теряем байт клавиатуры. */
        keyboard_push_byte(value);
    }

    return 0;
}

static void clamp_cursor(void)
{
    if (mouse_width == 0)
        mouse_width = 1;

    if (mouse_height == 0)
        mouse_height = 1;

    if (mx < 0)
        mx = 0;

    if (my < 0)
        my = 0;

    if ((uint32_t)mx >= mouse_width)
        mx = (int)(mouse_width - 1);

    if ((uint32_t)my >= mouse_height)
        my = (int)(mouse_height - 1);
}

void mouse_set_bounds(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    mouse_width = width;
    mouse_height = height;

    clamp_cursor();
}

void mouse_init(void)
{
    present = 0;
    packet_pos = 0;
    buttons = 0;

    if (!controller_command(0xA8))
        return;

    /*
     * Включаем вспомогательный порт.
     * Затем включаем передачу данных мышью.
     */
    if (!mouse_command(0xF6))
        return;

    if (!mouse_command(0xF4))
        return;

    mx = (int)(mouse_width / 2u);
    my = (int)(mouse_height / 2u);

    clamp_cursor();
    present = 1;
}

void mouse_poll(void)
{
    while (inb(PS2_STATUS_PORT) & STATUS_OUTPUT_FULL) {
        uint8_t status = inb(PS2_STATUS_PORT);
        uint8_t value = inb(PS2_DATA_PORT);

        if (!(status & STATUS_AUX_DATA)) {
            keyboard_push_byte(value);
            continue;
        }

        /*
         * Бит 3 должен быть установлен в первом байте
         * стандартного PS/2-пакета.
         */
        if (packet_pos == 0 && !(value & 0x08u))
            continue;

        packet[packet_pos++] = value;

        if (packet_pos < 3)
            continue;

        uint8_t flags = packet[0];

        /* Пропускаем пакеты с переполнением координат. */
        if (!(flags & 0xC0u)) {
            int dx = (int)(int8_t)packet[1];
            int dy = (int)(int8_t)packet[2];

            mx += dx;
            my -= dy;
        }

        buttons = flags & 0x07u;
        packet_pos = 0;

        clamp_cursor();
    }
}

int mouse_x(void)
{
    return mx;
}

int mouse_y(void)
{
    return my;
}

uint8_t mouse_buttons(void)
{
    return buttons;
}

int mouse_present(void)
{
    return present;
}

int mouse_left_pressed(void)
{
    return (buttons & MOUSE_LEFT) != 0;
}

int mouse_right_pressed(void)
{
    return (buttons & MOUSE_RIGHT) != 0;
}

int mouse_middle_pressed(void)
{
    return (buttons & MOUSE_MIDDLE) != 0;
}
