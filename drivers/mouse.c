#include "../include/mouse.h"
#include "../include/ports.h"

static MouseState mouse = {160, 100, 0, 0, 0};
static uint8_t packet[3];
static int packet_index = 0;

static void mouse_wait_write(void)
{
    int timeout = 100000;
    while (timeout-- && (inb(0x64) & 2)) {}
}

static void mouse_wait_read(void)
{
    int timeout = 100000;
    while (timeout-- && !(inb(0x64) & 1)) {}
}

static void mouse_write(uint8_t value)
{
    mouse_wait_write();
    outb(0x64, 0xD4);
    mouse_wait_write();
    outb(0x60, value);
}

static uint8_t mouse_read(void)
{
    mouse_wait_read();
    return inb(0x60);
}

void mouse_init(void)
{
    mouse_wait_write();
    outb(0x64, 0xA8);

    mouse_wait_write();
    outb(0x64, 0x20);
    mouse_wait_read();

    uint8_t status = inb(0x60);
    status |= 0x02;

    mouse_wait_write();
    outb(0x64, 0x60);
    mouse_wait_write();
    outb(0x60, status);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();
}

void mouse_poll(void)
{
    if (!(inb(0x64) & 1))
        return;

    if (!(inb(0x64) & 0x20))
        return;

    packet[packet_index++] = inb(0x60);

    if (packet_index != 3)
        return;

    packet_index = 0;

    int dx = (int)packet[1];
    int dy = (int)packet[2];

    if (packet[0] & 0x10) dx |= 0xFFFFFF00;
    if (packet[0] & 0x20) dy |= 0xFFFFFF00;

    mouse.x += dx;
    mouse.y -= dy;

    if (mouse.x < 0) mouse.x = 0;
    if (mouse.x > 319) mouse.x = 319;
    if (mouse.y < 0) mouse.y = 0;
    if (mouse.y > 199) mouse.y = 199;

    mouse.left   = (packet[0] & 1) != 0;
    mouse.right  = (packet[0] & 2) != 0;
    mouse.middle = (packet[0] & 4) != 0;
}

MouseState mouse_get_state(void)
{
    return mouse;
}
