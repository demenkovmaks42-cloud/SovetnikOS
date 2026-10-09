#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "mem.h"
#include "tar.h"

#define LINE_MAX 128

static char line[LINE_MAX];
static int  line_len = 0;

static void read_line(void) {
    line_len = 0;
    while (1) {
        char c = keyboard_getchar();
        if (c == '\n') {
            vga_putc('\n');
            line[line_len] = 0;
            return;
        }
        if (c == '\b') {
            if (line_len > 0) {
                line_len--;
                vga_print("\b \b");
            }
            continue;
        }
        if (line_len < LINE_MAX - 1) {
            line[line_len++] = c;
            vga_putc(c);
        }
    }
}

static void cmd_help(void) {
    vga_print("Commands:\n");
    vga_print("  help         - this help\n");
    vga_print("  about        - about sovetnikOS\n");
    vga_print("  clear        - clear screen\n");
    vga_print("  mem          - memory info\n");
    vga_print("  kmalloc N    - test alloc N bytes\n");
    vga_print("  ls           - list initrd files\n");
    vga_print("  cat NAME     - show file\n");
    vga_print("  reboot       - reboot\n");
}

static void cmd_about(void) {
    vga_print("sovetnikOS 0.2\n");
    vga_print("Minimal x86 educational OS\n");
}

static void cmd_mem(void) {
    vga_print("Total RAM: ");
    vga_print_dec(mem_total_ram() / 1024 / 1024);
    vga_print(" MB\n");
    vga_print("Heap start: ");
    vga_print_hex(mem_heap_start());
    vga_print("\nHeap end:   ");
    vga_print_hex(mem_heap_end());
    vga_print("\nHeap ptr:   ");
    vga_print_hex(mem_heap_ptr());
    vga_print("\n");
}

static void cmd_kmalloc(const char *arg) {
    uint32_t n = 0;
    while (*arg >= '0' && *arg <= '9') {
        n = n * 10 + (*arg - '0');
        arg++;
    }
    if (n == 0) { vga_print("usage: kmalloc N\n"); return; }
    void *p = kmalloc(n);
    if (!p) { vga_print("Out of memory\n"); return; }
    vga_print("Allocated ");
    vga_print_dec(n);
    vga_print(" bytes at ");
    vga_print_hex((uint32_t)p);
    vga_print("\n");
}

static void cmd_ls(void) {
    int n = tar_count();
    if (n == 0) { vga_print("(initrd empty)\n"); return; }
    for (int i = 0; i < n; i++) {
        tar_file_t *f = tar_get(i);
        vga_print(f->name);
        vga_print("  (");
        vga_print_dec(f->size);
        vga_print(" bytes)\n");
    }
}

static void cmd_cat(const char *name) {
    if (!name || !*name) { vga_print("usage: cat NAME\n"); return; }
    tar_file_t *f = tar_find(name);
    if (!f) { vga_print("File not found\n"); return; }
    vga_write(f->data, f->size);
    vga_putc('\n');
}

static void cmd_reboot(void) {
    __asm__ volatile ("outb %0, %1" :: "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64));
    while (1) __asm__ volatile ("hlt");
}

static void dispatch(char *cmd) {
    char *arg = cmd;
    while (*arg && *arg != ' ') arg++;
    if (*arg == ' ') { *arg = 0; arg++; while (*arg == ' ') arg++; }

    if (strcmp(cmd, "help") == 0)         cmd_help();
    else if (strcmp(cmd, "about") == 0)   cmd_about();
    else if (strcmp(cmd, "clear") == 0)   vga_clear();
    else if (strcmp(cmd, "mem") == 0)     cmd_mem();
    else if (strcmp(cmd, "kmalloc") == 0) cmd_kmalloc(arg);
    else if (strcmp(cmd, "ls") == 0)      cmd_ls();
    else if (strcmp(cmd, "cat") == 0)     cmd_cat(arg);
    else if (strcmp(cmd, "reboot") == 0)  cmd_reboot();
    else if (cmd[0] == 0)                 { /* пусто */ }
    else { vga_print("Unknown command: "); vga_print(cmd); vga_print("\n"); }
}

void shell_run(void) {
    vga_print("sovetnikOS shell. Type 'help'.\n");
    while (1) {
        vga_print("> ");
        read_line();
        dispatch(line);
    }
}
