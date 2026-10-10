#include "mouse.h"
#include "vga.h"

#define MOUSE_DATA   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_CMD    0x64

static mouse_state_t mouse = {
    32,
    32,
    0,
    0,
    0
};

static uint8_t packet[3];

static int packet_index = 0;
static int available = 0;

static inline void outb(
    uint16_t port,
    uint8_t value
)
{
    __asm__ volatile (
        "outb %0,%1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t inb(
    uint16_t port
)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1,%0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void wait_write(void)
{
    for (uint32_t i = 0;
         i < 100000;
    i++) {

        if (!(inb(MOUSE_STATUS) & 2))
            return;
    }
}

static int wait_read(void)
{
    for (uint32_t i = 0;
         i < 100000;
    i++) {

        if (inb(MOUSE_STATUS) & 1)
            return 0;
    }

    return -1;
}

static void mouse_write(
    uint8_t value
)
{
    wait_write();

    outb(
        MOUSE_CMD,
         0xD4
    );

    wait_write();

    outb(
        MOUSE_DATA,
         value
    );
}

static int mouse_read(
    uint8_t *value
)
{
    if (wait_read())
        return -1;

    *value =
    inb(MOUSE_DATA);

    return 0;
}

void mouse_init(void)
{
    /*
     * Enable auxiliary device.
     */
    wait_write();

    outb(
        MOUSE_CMD,
         0xA8
    );

    /*
     * Read controller command byte.
     */
    wait_write();

    outb(
        MOUSE_CMD,
         0x20
    );

    uint8_t status;

    if (mouse_read(&status))
        return;

    /*
     * Enable mouse data.
     *
     * IRQ12 remains disabled because
     * 0.7 uses polling.
     */
    status |= 0x02;
    status &= (uint8_t)~0x20;

    wait_write();

    outb(
        MOUSE_CMD,
         0x60
    );

    wait_write();

    outb(
        MOUSE_DATA,
         status
    );

    /*
     * Set defaults.
     */
    mouse_write(0xF6);

    if (mouse_read(&status))
        return;

    /*
     * Enable data reporting.
     */
    mouse_write(0xF4);

    if (mouse_read(&status))
        return;

    mouse.x =
    (int)(vga_width() / 2);

    mouse.y =
    (int)(vga_height() / 2);

    packet_index = 0;
    available = 1;
}

void mouse_poll(void)
{
    if (!available)
        return;

    while (inb(MOUSE_STATUS) & 1) {

        uint8_t status =
        inb(MOUSE_STATUS);

        /*
         * Ignore keyboard data.
         */
        if (!(status & 0x20)) {
            (void)inb(MOUSE_DATA);
            continue;
        }

        uint8_t byte =
        inb(MOUSE_DATA);

        /*
         * First byte always has bit 3 set.
         */
        if (packet_index == 0 &&
            !(byte & 0x08))
            continue;

        packet[packet_index++] =
        byte;

        if (packet_index < 3)
            continue;

        packet_index = 0;

        int dx =
        (int)packet[1];

        int dy =
        (int)packet[2];

        if (packet[0] & 0x10)
            dx -= 256;

        if (packet[0] & 0x20)
            dy -= 256;

        mouse.x += dx;
        mouse.y -= dy;

        if (mouse.x < 0)
            mouse.x = 0;

        if (mouse.y < 0)
            mouse.y = 0;

        if (mouse.x >= (int)vga_width())
            mouse.x =
            (int)vga_width() - 1;

        if (mouse.y >= (int)vga_height())
            mouse.y =
            (int)vga_height() - 1;

        mouse.left =
        (packet[0] & 1) != 0;

        mouse.right =
        (packet[0] & 2) != 0;

        mouse.middle =
        (packet[0] & 4) != 0;
    }
}

mouse_state_t mouse_get_state(void)
{
    return mouse;
}

int mouse_available(void)
{
    return available;
}
