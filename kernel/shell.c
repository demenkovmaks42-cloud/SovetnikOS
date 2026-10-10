#include <stdint.h>
#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "mem.h"
#include "ramfs.h"
#include "ata.h"
#include "pit.h"
#include "pic.h"

#define LINE_SIZE 128
#define COLOR_NORMAL 0x00C0C0C0U
#define COLOR_PROMPT 0x0000A0FFU
#define COLOR_ERROR  0x00FF5050U
#define COLOR_OK     0x0050FF50U
#define COLOR_INFO   0x00FFFFFFU

static char input_line[LINE_SIZE];

static void print_error(const char *s)
{
    vga_set_fg(COLOR_ERROR);
    vga_print(s);
    vga_set_fg(COLOR_NORMAL);
}

static void copy_string(char *dst, const char *src)
{
    while ((*dst++ = *src++) != '\0') { }
}

static int parse_u32(const char *s, uint32_t *out)
{
    uint32_t n = 0;
    int digits = 0;
    while (*s == ' ') ++s;
    while (*s >= '0' && *s <= '9') {
        uint32_t d = (uint32_t)(*s - '0');
        if (n > (UINT32_MAX - d) / 10U) return 0;
        n = n * 10U + d;
        ++s;
        digits = 1;
    }
    while (*s == ' ') ++s;
    if (!digits || *s != '\0') return 0;
    *out = n;
    return 1;
}

static void read_line(void)
{
    uint32_t n = 0;
    vga_set_fg(COLOR_PROMPT);
    vga_print("sovetnikOS:/ ");
    vga_set_fg(COLOR_NORMAL);

    for (;;) {
        char c = keyboard_getchar();
        if (c == '\r') c = '\n';
        if (c == '\n') {
            input_line[n] = '\0';
            vga_putc('\n');
            return;
        }
        if (c == '\b' || c == 127) {
            if (n > 0) {
                --n;
                vga_print("\b \b");
            }
            continue;
        }
        if ((unsigned char)c >= 32 && (unsigned char)c < 127 && n < LINE_SIZE - 1) {
            input_line[n++] = c;
            vga_putc(c);
        }
    }
}

static void read_text(char *buf, uint32_t cap, uint32_t *length, int append)
{
    uint32_t n = append ? *length : 0;
    if (n >= cap) n = cap - 1;
    vga_print("Enter text; type a single dot (.) on a line to save.\n");
    while (n + 1 < cap) {
        char line[LINE_SIZE];
        uint32_t m = 0;
        vga_print("... ");
        for (;;) {
            char c = keyboard_getchar();
            if (c == '\r') c = '\n';
            if (c == '\n') {
                line[m] = '\0';
                vga_putc('\n');
                break;
            }
            if (c == '\b' || c == 127) {
                if (m) { --m; vga_print("\b \b"); }
                continue;
            }
            if ((unsigned char)c >= 32 && (unsigned char)c < 127 && m < LINE_SIZE - 1) {
                line[m++] = c;
                vga_putc(c);
            }
        }
        if (m == 1 && line[0] == '.') break;
        if (n + m + 1 >= cap) {
            print_error("File is full; remaining text was not saved.\n");
            break;
        }
        for (uint32_t i = 0; i < m; ++i) buf[n++] = line[i];
        buf[n++] = '\n';
    }
    buf[n] = '\0';
    *length = n;
}

static int resolve_path(const char *arg, char *out)
{
    if (!arg || !*arg) return 0;
    return ramfs_resolve(arg, out) == 0;
}

static void cmd_help(void)
{
    vga_print("Commands:\n"
    " help, about, clear, reboot\n"
    " ticks, mem, kmalloc N\n"
    " ls, pwd, cd PATH, mkdir NAME, rmdir NAME\n"
    " touch NAME, cat NAME, write NAME, append NAME, rm NAME\n"
    " sov NAME, sp NAME\n"
    " ata-info, ata-read LBA, ata-write LBA\n");
}

static void cmd_mem(void)
{
    vga_print("Heap start: "); vga_print_hex((uint32_t)mem_heap_start);
    vga_print("\nHeap end:   "); vga_print_hex((uint32_t)mem_heap_end);
    vga_print("\nHeap ptr:   "); vga_print_hex((uint32_t)mem_heap_ptr);
    vga_print("\nHeap used:  ");
    vga_print_dec((uint32_t)(mem_heap_ptr >= mem_heap_start ? mem_heap_ptr - mem_heap_start : 0));
    vga_print(" bytes\nHeap free:  ");
    vga_print_dec((uint32_t)(mem_heap_end >= mem_heap_ptr ? mem_heap_end - mem_heap_ptr : 0));
    vga_print(" bytes\n");
}

static void cmd_ls(void)
{
    const char *cwd = ramfs_cwd();
    uint32_t cwd_len = (uint32_t)strlen(cwd);
    int shown = 0;
    for (int i = 0; i < ramfs_count(); ++i) {
        const char *name = ramfs_name_at(i);
        if (!name || strncmp(name, cwd, cwd_len) != 0) continue;
        const char *rest = name + cwd_len;
        if (*rest == '/') ++rest;
        if (!*rest) continue;
        int direct = 1;
        for (const char *p = rest; *p; ++p) if (*p == '/') { direct = 0; break; }
        if (!direct) continue;
        if (ramfs_type_at(i) == RAMFS_TYPE_DIR) vga_print("[dir]  ");
        else vga_print("[file] ");
        vga_print(rest);
        if (ramfs_type_at(i) != RAMFS_TYPE_DIR) {
            vga_print(" ("); vga_print_dec(ramfs_size_at(i)); vga_print(" bytes)");
        }
        vga_putc('\n');
        shown++;
    }
    if (!shown) vga_print("(empty)\n");
}

static void cmd_cat(const char *arg)
{
    char path[RAMFS_PATH_MAX];
    uint32_t size = 0;
    if (!resolve_path(arg, path)) { print_error("Usage: cat NAME\n"); return; }
    const char *data = ramfs_read(path, &size);
    if (!data) { print_error("File not found.\n"); return; }
    vga_write(data, size);
    if (!size || data[size - 1] != '\n') vga_putc('\n');
}

static void cmd_touch(const char *arg)
{
    char path[RAMFS_PATH_MAX];
    if (!resolve_path(arg, path)) { print_error("Usage: touch NAME\n"); return; }
    if (ramfs_exists(path)) { print_error("Already exists.\n"); return; }
    if (ramfs_create(path, RAMFS_TYPE_FILE) != 0) { print_error("Cannot create file.\n"); return; }
    vga_set_fg(COLOR_OK); vga_print("Created: "); vga_print(path); vga_putc('\n'); vga_set_fg(COLOR_NORMAL);
}

static void cmd_write(const char *arg, int append)
{
    char path[RAMFS_PATH_MAX];
    char data[RAMFS_MAX_FILE_SIZE];
    uint32_t size = 0;
    if (!resolve_path(arg, path)) { print_error("Usage: write NAME\n"); return; }
    int exists = ramfs_exists(path);
    if (exists && ramfs_is_dir(path)) { print_error("Is a directory.\n"); return; }
    if (!exists && ramfs_create(path, RAMFS_TYPE_FILE) != 0) { print_error("Cannot create file.\n"); return; }
    if (append && exists) {
        uint32_t old_size = 0;
        const char *old = ramfs_read(path, &old_size);
        if (old) {
            if (old_size >= RAMFS_MAX_FILE_SIZE) { print_error("File is already full.\n"); return; }
            memcpy(data, old, old_size);
            size = old_size;
        }
    }
    read_text(data, RAMFS_MAX_FILE_SIZE, &size, append);
    if (ramfs_write(path, data, size) != 0) { print_error("Write failed.\n"); return; }
    vga_set_fg(COLOR_OK); vga_print("Saved "); vga_print_dec(size); vga_print(" bytes to "); vga_print(path); vga_putc('\n'); vga_set_fg(COLOR_NORMAL);
}

static void cmd_rm(const char *arg)
{
    char path[RAMFS_PATH_MAX];
    if (!resolve_path(arg, path)) { print_error("Usage: rm NAME\n"); return; }
    if (!ramfs_exists(path)) { print_error("Not found.\n"); return; }
    if (ramfs_is_dir(path)) { print_error("Is a directory; use rmdir.\n"); return; }
    if (ramfs_delete(path) != 0) { print_error("Delete failed.\n"); return; }
    vga_print("Deleted: "); vga_print(path); vga_putc('\n');
}

static void cmd_cd(const char *arg)
{
    char path[RAMFS_PATH_MAX];
    if (!arg || !*arg) { ramfs_set_cwd("/"); return; }
    if (strcmp(arg, "..") == 0) {
        char parent[RAMFS_PATH_MAX];
        const char *cwd = ramfs_cwd();
        strncpy(parent, cwd, RAMFS_PATH_MAX - 1);
        parent[RAMFS_PATH_MAX - 1] = '\0';
        uint32_t n = (uint32_t)strlen(parent);
        while (n > 1 && parent[n - 1] != '/') --n;
        if (n > 1) parent[n - 1] = '\0'; else parent[1] = '\0';
        ramfs_set_cwd(parent);
        return;
    }
    if (!resolve_path(arg, path) || !ramfs_exists(path) || !ramfs_is_dir(path)) {
        print_error("No such directory.\n"); return;
    }
    ramfs_set_cwd(path);
}

static void cmd_mkdir(const char *arg, int directory)
{
    char path[RAMFS_PATH_MAX];
    if (!resolve_path(arg, path)) { print_error(directory ? "Usage: mkdir NAME\n" : "Usage: rmdir NAME\n"); return; }
    if (directory) {
        if (ramfs_exists(path)) { print_error("Already exists.\n"); return; }
        if (ramfs_create(path, RAMFS_TYPE_DIR) != 0) { print_error("Cannot create directory.\n"); return; }
    } else {
        if (!ramfs_exists(path) || !ramfs_is_dir(path)) { print_error("Not a directory.\n"); return; }
        uint32_t n = (uint32_t)strlen(path);
        for (int i = 0; i < ramfs_count(); ++i) {
            const char *name = ramfs_name_at(i);
            if (name && strncmp(name, path, n) == 0 && name[n] == '/') { print_error("Directory not empty.\n"); return; }
        }
        if (ramfs_delete(path) != 0) { print_error("Remove failed.\n"); return; }
    }
    vga_print(directory ? "Directory created: " : "Directory removed: "); vga_print(path); vga_putc('\n');
}

static void cmd_ata_info(void)
{
    if (ata_init() != 0) { print_error("No ATA primary-master disk detected.\n"); return; }
    vga_print("Model: "); vga_print(ata_model());
    vga_print("\nSectors: "); vga_print_dec(ata_sectors()); vga_putc('\n');
}

static void cmd_ata_read(const char *arg)
{
    uint32_t lba;
    uint8_t sector[ATA_SECTOR_SIZE];
    if (!parse_u32(arg, &lba)) { print_error("Usage: ata-read LBA\n"); return; }
    if (ata_read_sectors(lba, 1, sector) != 0) { print_error("ATA read failed.\n"); return; }
    for (uint32_t i = 0; i < 128; ++i) {
        const char *hex = "0123456789ABCDEF";
        if ((i % 16) == 0) { vga_print_hex(i); vga_print("  "); }
        vga_putc(hex[sector[i] >> 4]); vga_putc(hex[sector[i] & 15]); vga_putc(' ');
        if ((i % 16) == 15) vga_putc('\n');
    }
}

static void cmd_ata_write(const char *arg)
{
    uint32_t lba;
    uint8_t sector[ATA_SECTOR_SIZE];
    char text[ATA_SECTOR_SIZE];
    uint32_t len = 0;
    if (!parse_u32(arg, &lba)) { print_error("Usage: ata-write LBA\n"); return; }
    if (ata_read_sectors(lba, 1, sector) != 0) { print_error("ATA read failed.\n"); return; }
    vga_print("Enter replacement text; single dot on a line saves.\n");
    read_text(text, ATA_SECTOR_SIZE, &len, 0);
    for (uint32_t i = 0; i < ATA_SECTOR_SIZE; ++i) sector[i] = i < len ? (uint8_t)text[i] : 0;
    if (ata_write_sectors(lba, 1, sector) != 0) { print_error("ATA write failed.\n"); return; }
    vga_print("Sector written.\n");
}

static void dispatch(char *cmd)
{
    char *arg = cmd;
    while (*arg && *arg != ' ') ++arg;
    if (*arg) { *arg++ = '\0'; while (*arg == ' ') ++arg; }

    if (strcmp(cmd, "") == 0) return;
    if (strcmp(cmd, "help") == 0) cmd_help();
    else if (strcmp(cmd, "about") == 0) vga_print("sovetnikOS - minimal x86 operating system\n");
    else if (strcmp(cmd, "clear") == 0) vga_clear();
    else if (strcmp(cmd, "reboot") == 0) { __asm__ volatile ("cli"); while (1) __asm__ volatile ("outb %0, %1" :: "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64)); }
    else if (strcmp(cmd, "ticks") == 0) { vga_print("Ticks: "); vga_print_dec(pit_ticks); vga_putc('\n'); }
    else if (strcmp(cmd, "mem") == 0) cmd_mem();
    else if (strcmp(cmd, "kmalloc") == 0) {
        uint32_t n;
        if (!parse_u32(arg, &n) || n == 0) { print_error("Usage: kmalloc N\n"); return; }
        void *p = mem_alloc(n);
        if (!p) { print_error("Out of memory or heap not initialized.\n"); return; }
        vga_print("Allocated "); vga_print_dec(n); vga_print(" bytes at "); vga_print_hex((uint32_t)(uintptr_t)p); vga_putc('\n');
    }
    else if (strcmp(cmd, "ls") == 0) cmd_ls();
    else if (strcmp(cmd, "pwd") == 0) { vga_print(ramfs_cwd()); vga_putc('\n'); }
    else if (strcmp(cmd, "cd") == 0) cmd_cd(arg);
    else if (strcmp(cmd, "mkdir") == 0) cmd_mkdir(arg, 1);
    else if (strcmp(cmd, "rmdir") == 0) cmd_mkdir(arg, 0);
    else if (strcmp(cmd, "cat") == 0) cmd_cat(arg);
    else if (strcmp(cmd, "touch") == 0) cmd_touch(arg);
    else if (strcmp(cmd, "write") == 0 || strcmp(cmd, "edit") == 0 || strcmp(cmd, "sov") == 0 || strcmp(cmd, "sp") == 0) {
        char path[RAMFS_PATH_MAX];
        char name[RAMFS_PATH_MAX];
        const char *ext = (strcmp(cmd, "sov") == 0) ? ".sov" : ((strcmp(cmd, "sp") == 0) ? ".sp" : "");
        if (!arg || !*arg) { print_error("Usage: write NAME\n"); return; }
        if (*ext) {
            size_t alen = strlen(arg), elen = strlen(ext);
            if (alen + elen >= sizeof(name)) { print_error("Name too long.\n"); return; }
            copy_string(name, arg);
            int has_ext = alen >= elen && strcmp(name + alen - elen, ext) == 0;
            if (!has_ext) copy_string(name + alen, ext);
            arg = name;
        }
        if (!resolve_path(arg, path)) { print_error("Invalid path.\n"); return; }
        /* Reuse the normal file editor by passing the original argument. */
        cmd_write(arg, 0);
    }
    else if (strcmp(cmd, "append") == 0) cmd_write(arg, 1);
    else if (strcmp(cmd, "rm") == 0) cmd_rm(arg);
    else if (strcmp(cmd, "ata-info") == 0) cmd_ata_info();
    else if (strcmp(cmd, "ata-read") == 0) cmd_ata_read(arg);
    else if (strcmp(cmd, "ata-write") == 0) cmd_ata_write(arg);
    else { vga_print("Unknown command: "); vga_print(cmd); vga_putc('\n'); }
}

void shell_run(void)
{
    vga_set_fg(COLOR_INFO);
    vga_print("sovetnikOS shell. Type 'help'.\n");
    vga_set_fg(COLOR_NORMAL);
    for (;;) {
        read_line();
        dispatch(input_line);
    }
}
