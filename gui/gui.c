
#include "gui.h"
#include "vga.h"
#include "mouse.h"
#include "keyboard.h"
#include "types.h"

#define C_DESKTOP 0x001B2638
#define C_PANEL   0x002B3B50
#define C_WINDOW  0x002F4054
#define C_TITLE   0x004B6584
#define C_TEXT    0x00FFFFFF
#define C_ACCENT  0x0038BDF8
#define C_ICON    0x00344A60
#define C_CLOSE   0x00E74C3C

static void cursor_draw(
    int x,
    int y
)
{
    if (!vga_is_framebuffer())
        return;

    /*
     * Small graphical arrow cursor.
     */
    for (int i = 0;
         i < 12;
    i++) {

        vga_draw_pixel(
            (uint32_t)(x + i),
                       (uint32_t)(y + i),
                       C_TEXT
        );

        if (i < 8) {

            vga_draw_pixel(
                (uint32_t)(x + i),
                           (uint32_t)(y + i + 1),
                           C_TEXT
            );
        }
    }

    for (int i = 0;
         i < 7;
    i++) {

        vga_draw_pixel(
            (uint32_t)(x + i),
                       (uint32_t)(y + 14),
                       C_TEXT
        );
    }
}

static void draw_text(
    uint32_t x,
    uint32_t y,
    uint32_t color,
    const char *text
)
{
    vga_set_cursor_pixel(
        x,
        y
    );

    vga_set_fg(color);

    vga_print(text);
}

void gui_init(void)
{
    if (!vga_is_framebuffer())
        return;

    uint32_t width =
    vga_width();

    uint32_t height =
    vga_height();

    /*
     * Desktop.
     */
    vga_set_bg(
        C_DESKTOP
    );

    vga_draw_rect(
        0,
        0,
        width,
        height,
        C_DESKTOP
    );

    /*
     * Top panel.
     */
    vga_draw_rect(
        0,
        0,
        width,
        40,
        C_PANEL
    );

    draw_text(
        16,
        12,
        C_TEXT,
        "sovetnikOS"
    );

    if (width > 100) {

        draw_text(
            width - 88,
            12,
            C_TEXT,
            "0.7"
        );
    }

    /*
     * Bottom taskbar.
     */
    vga_draw_rect(
        0,
        height - 44,
        width,
        44,
        C_PANEL
    );

    vga_draw_rect(
        12,
        height - 36,
        26,
        26,
        C_ACCENT
    );

    draw_text(
        50,
        height - 33,
        C_TEXT,
        "Start"
    );

    /*
     * Terminal window.
     */
    uint32_t wx = 80;
    uint32_t wy = 70;

    uint32_t ww =
    (width > 760) ?
    width - 160 :
    width - 40;

    uint32_t wh =
    (height > 600) ?
    height - 150 :
    height - 100;

    vga_draw_rect(
        wx,
        wy,
        ww,
        wh,
        C_WINDOW
    );

    /*
     * Window title.
     */
    vga_draw_rect(
        wx,
        wy,
        ww,
        34,
        C_TITLE
    );

    /*
     * Close button.
     */
    vga_draw_rect(
        wx + ww - 30,
        wy + 8,
        16,
        16,
        C_CLOSE
    );

    draw_text(
        wx + 12,
        wy + 9,
        C_TEXT,
        "SovTerminal"
    );

    draw_text(
        wx + 20,
        wy + 54,
        C_ACCENT,
        "Graphical desktop initialized."
    );

    draw_text(
        wx + 20,
        wy + 78,
        C_TEXT,
        "Mouse: PS/2"
    );

    draw_text(
        wx + 20,
        wy + 102,
        C_TEXT,
        "Keyboard: PS/2"
    );

    draw_text(
        wx + 20,
        wy + 126,
        C_TEXT,
        "Press ESC to return to shell."
    );

    /*
     * Desktop icons.
     */
    vga_draw_rect(
        24,
        80,
        48,
        48,
        C_ICON
    );

    draw_text(
        26,
        136,
        C_TEXT,
        "Files"
    );

    vga_draw_rect(
        24,
        175,
        48,
        48,
        C_ICON
    );

    draw_text(
        20,
        231,
        C_TEXT,
        "Settings"
    );

    vga_draw_rect(
        24,
        270,
        48,
        48,
        C_ICON
    );

    draw_text(
        26,
        326,
        C_TEXT,
        "Info"
    );
}

void gui_run(void)
{
    if (!vga_is_framebuffer()) {

        vga_print(
            "GUI requires a Multiboot framebuffer.\n"
        );

        return;
    }

    gui_init();

    mouse_state_t old =
    mouse_get_state();

    cursor_draw(
        old.x,
        old.y
    );

    for (;;) {

        mouse_poll();

        mouse_state_t current =
        mouse_get_state();

        if (current.x != old.x ||
            current.y != old.y ||
            current.left != old.left ||
            current.right != old.right ||
            current.middle != old.middle) {

            /*
             * Redraw background to remove
             * the previous cursor.
             */
            gui_init();

        cursor_draw(
            current.x,
            current.y
        );

        old = current;
            }

            /*
             * keyboard_getkey() polls mouse
             * while waiting.
             */
            int key =
            keyboard_getkey();

            if (key == 27)
                break;

        /*
         * Press C to redraw desktop.
         */
        if (key == 'c') {

            gui_init();

            cursor_draw(
                current.x,
                current.y
            );
        }
    }

    vga_set_bg(
        0x00000000
    );

    vga_clear();

    vga_set_fg(
        0x00C0C0C0
    );
}
