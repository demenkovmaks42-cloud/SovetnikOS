#include "../include/kernel.h"
#include "../include/ports.h"
#include "../include/keyboard.h"
#include "../include/mouse.h"
#include "../include/console.h"
#include "../include/gui.h"

static char command[64];
static int command_len = 0;

static int str_eq(const char *a, const char *b)
{
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == 0 && b[i] == 0;
}

static void command_run(void)
{
    command[command_len] = 0;

    console_putc('\n');

    if (str_eq(command, "help")) {
        console_puts("Commands:\n");
        console_puts("  help   - show commands\n");
        console_puts("  about  - system information\n");
        console_puts("  clear  - clear screen\n");
        console_puts("  gui    - show GUI information\n");
        console_puts("  mem    - show memory information\n");
        console_puts("  reboot - reboot the machine\n");
    } else if (str_eq(command, "about")) {
        console_puts("sovetnikOS 0.1\n");
        console_puts("Educational x86 operating system.\n");
        console_puts("Kernel: C, boot: NASM/GRUB.\n");
    } else if (str_eq(command, "clear")) {
        console_clear();
    } else if (str_eq(command, "gui")) {
        console_puts("GUI subsystem: initialized.\n");
        console_puts("Display backend: VGA text mode in 0.1.\n");
        console_puts("Framebuffer GUI is planned for 0.2.\n");
    } else if (str_eq(command, "mem")) {
        console_puts("Kernel memory base: 0x00100000\n");
        console_puts("Stack: 16 KiB static kernel stack.\n");
    } else if (str_eq(command, "reboot")) {
        console_puts("Rebooting...\n");
        outb(0x64, 0xFE);
        for (;;) __asm__ volatile ("hlt");
    } else if (command_len != 0) {
        console_puts("Unknown command. Type 'help'.\n");
    }

    command_len = 0;
    command[0] = 0;
    console_prompt();
}

void kernel_main(unsigned int magic, unsigned int multiboot_info)
{
    (void)magic;
    (void)multiboot_info;

    gui_init();
    keyboard_init();
    mouse_init();

    console_prompt();

    for (;;) {
        mouse_poll();

        if (keyboard_has_data()) {
            uint8_t scan = keyboard_read_scancode();

            if (scan & 0x80)
                continue;

            char c = keyboard_scancode_to_ascii(scan);

            if (!c)
                continue;

            if (c == '\n') {
                command_run();
            } else if (c == '\b') {
                if (command_len > 0) {
                    command_len--;
                    console_putc('\b');
                }
            } else if (command_len < (int)sizeof(command) - 1) {
                command[command_len++] = c;
                console_putc(c);
            }
        }
    }
}
