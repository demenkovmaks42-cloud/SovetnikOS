#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "mem.h"
#include "tar.h"
#include "ramfs.h"
#include "ata.h"

#define LINE_MAX 128

static char line[LINE_MAX];
static int  line_len = 0;

/* ==================== ВВОД ==================== */

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

static int read_multiline(char *buf, uint32_t max, int clear_first) {
    uint32_t pos = 0;
    if (clear_first) buf[0] = 0;

    while (1) {
        vga_print("... ");
        line_len = 0;
        while (1) {
            char c = keyboard_getchar();
            if (c == '\n') { vga_putc('\n'); line[line_len] = 0; break; }
            if (c == '\b') {
                if (line_len > 0) { line_len--; vga_print("\b \b"); }
                continue;
            }
            if (line_len < LINE_MAX - 1) {
                line[line_len++] = c;
                vga_putc(c);
            }
        }

        if (line[0] == '.' && line[1] == 0) return (int)pos;

        uint32_t n = strlen(line);
        for (uint32_t i = 0; i < n; i++) {
            if (pos + 1 >= max) return (int)pos;
            buf[pos++] = line[i];
        }
        if (pos + 1 >= max) return (int)pos;
        buf[pos++] = '\n';
        buf[pos] = 0;
    }
}

/* ==================== УТИЛИТЫ ==================== */

static void print_hex_byte(uint8_t b) {
    const char *hex = "0123456789ABCDEF";
    vga_putc(hex[(b >> 4) & 0xF]);
    vga_putc(hex[b & 0xF]);
}

static void hexdump(const uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i += 16) {
        const char *hex = "0123456789ABCDEF";
        for (int s = 28; s >= 0; s -= 4) vga_putc(hex[(i >> s) & 0xF]);
        vga_print("  ");

        for (uint32_t j = 0; j < 16; j++) {
            if (i + j < len) print_hex_byte(buf[i + j]);
            else             vga_print("  ");
            vga_putc(' ');
        }
        vga_putc(' ');

        for (uint32_t j = 0; j < 16; j++) {
            if (i + j >= len) break;
            uint8_t c = buf[i + j];
            vga_putc((c >= 32 && c < 127) ? (char)c : '.');
        }
        vga_putc('\n');
    }
}

/* Проверяет, оканчивается ли строка на ext (без учёта регистра) */
static int has_ext(const char *name, const char *ext) {
    int n = strlen(name);
    int e = strlen(ext);
    if (n < e) return 0;
    for (int i = 0; i < e; i++) {
        char a = name[n - e + i];
        char b = ext[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return 0;
    }
    return 1;
}

/* Делает "name.ext", если расширения ещё нет */
static void add_ext(const char *arg, const char *ext,
                    char *out, int out_max) {
    if (has_ext(arg, ext)) {
        strncpy(out, arg, out_max - 1);
        out[out_max - 1] = 0;
        return;
    }
    int i = 0;
    while (arg[i] && i < out_max - 1) { out[i] = arg[i]; i++; }
    int j = 0;
    while (ext[j] && i < out_max - 1) { out[i++] = ext[j++]; }
    out[i] = 0;
                    }

                    /* ==================== КОМАНДЫ: ОБЩИЕ ==================== */

                    static void cmd_help(void) {
                        vga_print("Commands:\n");
                        vga_print("  help              - this help\n");
                        vga_print("  about             - about sovetnikOS\n");
                        vga_print("  clear             - clear screen\n");
                        vga_print("  reboot            - reboot\n");
                        vga_print("  mem               - memory info\n");
                        vga_print("  kmalloc N         - test alloc N bytes\n");
                        vga_print("-- files --\n");
                        vga_print("  ls                - list files in cwd\n");
                        vga_print("  cat NAME          - show file\n");
                        vga_print("  touch NAME        - create empty file\n");
                        vga_print("  write NAME        - write file (end with '.')\n");
                        vga_print("  append NAME       - append to file (end with '.')\n");
                        vga_print("  edit NAME         - overwrite file (end with '.')\n");
                        vga_print("  rm NAME           - delete file\n");
                        vga_print("-- sovetnik formats --\n");
                        vga_print("  sov NAME          - edit .sov (text file)\n");
                        vga_print("  sp  NAME          - edit .sp  (system file)\n");
                        vga_print("-- directories --\n");
                        vga_print("  pwd               - print working directory\n");
                        vga_print("  cd [DIR]          - change directory (cd .. to go up)\n");
                        vga_print("  mkdir NAME        - create directory\n");
                        vga_print("  rmdir NAME        - remove empty directory\n");
                        vga_print("-- ATA disk --\n");
                        vga_print("  ata-info          - show disk info\n");
                        vga_print("  ata-read LBA      - read one sector (hexdump)\n");
                        vga_print("  ata-write LBA     - write text into sector (end with '.')\n");
                    }

                    static void cmd_about(void) {
                        vga_print("sovetnikOS 0.4\n");
                        vga_print("Minimal x86 educational OS\n");
                        vga_print("RAM-disk with folders, .sov/.sp formats, ATA PIO driver\n");
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
                        while (*arg >= '0' && *arg <= '9') { n = n * 10 + (*arg - '0'); arg++; }
                        if (n == 0) { vga_print("usage: kmalloc N\n"); return; }
                        void *p = kmalloc(n);
                        if (!p) { vga_print("Out of memory\n"); return; }
                        vga_print("Allocated ");
                        vga_print_dec(n);
                        vga_print(" bytes at ");
                        vga_print_hex((uint32_t)p);
                        vga_print("\n");
                    }

                    /* ==================== КОМАНДЫ: ФАЙЛЫ ==================== */

                    static void cmd_ls(void) {
                        const char *cur = ramfs_cwd();
                        int cur_len = strlen(cur);
                        int shown = 0;

                        for (int i = 0; i < ramfs_count(); i++) {
                            const char *name = ramfs_name_at(i);
                            if (!name) continue;
                            if (strncmp(name, cur, cur_len) != 0) continue;
                            const char *rest = name + cur_len;
                            if (*rest == '/') rest++;
                            if (*rest == 0) continue;
                            int direct = 1;
                            for (const char *p = rest; *p; p++) {
                                if (*p == '/') { direct = 0; break; }
                            }
                            if (!direct) continue;

                            if (ramfs_type_at(i) == RAMFS_TYPE_DIR) {
                                vga_print("[dir]  ");
                            } else {
                                int sub = ramfs_subtype_at(i);
                                if      (sub == RAMFS_SUB_SOV) vga_print("[sov]  ");
                                else if (sub == RAMFS_SUB_SP)  vga_print("[sp]   ");
                                else                           vga_print("       ");
                            }
                            vga_print(rest);
                            if (ramfs_type_at(i) != RAMFS_TYPE_DIR) {
                                vga_print("  (");
                                vga_print_dec(ramfs_size_at(i));
                                vga_print(" b)");
                            }
                            vga_print("\n");
                            shown++;
                        }
                        if (shown == 0) vga_print("(empty)\n");
                    }

                    static void cmd_cat(const char *arg) {
                        if (!arg || !*arg) { vga_print("usage: cat NAME\n"); return; }
                        char full[RAMFS_PATH_MAX];
                        ramfs_resolve(arg, full);
                        uint32_t size;
                        const char *data = ramfs_read(full, &size);
                        if (!data) { vga_print("File not found\n"); return; }

                        if (ramfs_ext_type(full) == RAMFS_SUB_SP) {
                            vga_print("--- .sp file (");
                            vga_print_dec(size);
                            vga_print(" bytes) ---\n");
                            vga_write(data, size);
                            if (size == 0 || data[size - 1] != '\n') vga_putc('\n');
                            vga_print("--- hex ---\n");
                            hexdump((const uint8_t*)data, size > 256 ? 256 : size);
                        } else {
                            vga_write(data, size);
                            if (size == 0 || data[size - 1] != '\n') vga_putc('\n');
                        }
                    }

                    static void cmd_touch(const char *arg) {
                        if (!arg || !*arg) { vga_print("usage: touch NAME\n"); return; }
                        char full[RAMFS_PATH_MAX];
                        ramfs_resolve(arg, full);
                        if (ramfs_exists(full)) { vga_print("Already exists\n"); return; }
                        if (ramfs_create(full, RAMFS_TYPE_FILE) != 0) {
                            vga_print("Cannot create file\n");
                            return;
                        }
                        vga_print("Created: ");
                        vga_print(full);
                        vga_print("\n");
                    }

                    static void do_write(const char *arg, int append_mode) {
                        if (!arg || !*arg) { vga_print("usage: write NAME\n"); return; }
                        char full[RAMFS_PATH_MAX];
                        ramfs_resolve(arg, full);

                        int existed = ramfs_exists(full);
                        if (!existed) {
                            if (ramfs_create(full, RAMFS_TYPE_FILE) != 0) {
                                vga_print("Cannot create file\n");
                                return;
                            }
                        } else if (ramfs_is_dir(full)) {
                            vga_print("Is a directory\n");
                            return;
                        }

                        char buf[RAMFS_MAX_FILE_SIZE];
                        uint32_t start_size = 0;
                        if (append_mode && existed) {
                            uint32_t sz;
                            const char *old = ramfs_read(full, &sz);
                            if (old && sz > 0) {
                                if (sz > RAMFS_MAX_FILE_SIZE) sz = RAMFS_MAX_FILE_SIZE;
                                memcpy(buf, old, sz);
                                start_size = sz;
                            }
                        }

                        vga_print("Enter text. End with '.' on empty line:\n");
                        int added = read_multiline(buf + start_size,
                                                   RAMFS_MAX_FILE_SIZE - start_size, 0);
                        uint32_t total = start_size + (uint32_t)added;

                        if (ramfs_write(full, buf, total) != 0) {
                            vga_print("Write failed\n");
                            return;
                        }
                        vga_print("Saved ");
                        vga_print_dec(total);
                        vga_print(" bytes to ");
                        vga_print(full);
                        vga_print("\n");
                    }

                    static void cmd_write(const char *arg)  { do_write(arg, 0); }
                    static void cmd_append(const char *arg) { do_write(arg, 1); }
                    static void cmd_edit(const char *arg)   { do_write(arg, 0); }

                    static void cmd_rm(const char *arg) {
                        if (!arg || !*arg) { vga_print("usage: rm NAME\n"); return; }
                        char full[RAMFS_PATH_MAX];
                        ramfs_resolve(arg, full);
                        if (!ramfs_exists(full)) { vga_print("File not found\n"); return; }
                        if (ramfs_is_dir(full))  { vga_print("Is a directory (use rmdir)\n"); return; }
                        ramfs_delete(full);
                        vga_print("Deleted: ");
                        vga_print(full);
                        vga_print("\n");
                    }

                    /* ==================== КОМАНДЫ: ДИРЕКТОРИИ ==================== */

                    static void cmd_pwd(void) {
                        vga_print(ramfs_cwd());
                        vga_print("\n");
                    }

                    static void cmd_cd(const char *arg) {
                        if (!arg || !*arg) { ramfs_set_cwd("/"); return; }

                        if (strcmp(arg, "..") == 0) {
                            char full[RAMFS_PATH_MAX];
                            strncpy(full, ramfs_cwd(), RAMFS_PATH_MAX - 1);
                            full[RAMFS_PATH_MAX - 1] = 0;
                            int len = strlen(full);
                            if (len <= 1) { ramfs_set_cwd("/"); return; }
                            int i = len - 1;
                            while (i > 0 && full[i] != '/') i--;
                            if (i == 0) full[1] = 0;
                            else        full[i] = 0;
                            ramfs_set_cwd(full);
                            return;
                        }

                        char full[RAMFS_PATH_MAX];
                        ramfs_resolve(arg, full);

                        if (!ramfs_exists(full) || !ramfs_is_dir(full)) {
                            vga_print("No such directory\n");
                            return;
                        }
                        ramfs_set_cwd(full);
                    }

                    static void cmd_mkdir(const char *arg) {
                        if (!arg || !*arg) { vga_print("usage: mkdir NAME\n"); return; }
                        char full[RAMFS_PATH_MAX];
                        ramfs_resolve(arg, full);
                        if (ramfs_exists(full)) { vga_print("Already exists\n"); return; }
                        if (ramfs_create(full, RAMFS_TYPE_DIR) != 0) {
                            vga_print("Cannot create directory\n");
                            return;
                        }
                        vga_print("Created: ");
                        vga_print(full);
                        vga_print("\n");
                    }

                    static void cmd_rmdir(const char *arg) {
                        if (!arg || !*arg) { vga_print("usage: rmdir NAME\n"); return; }
                        char full[RAMFS_PATH_MAX];
                        ramfs_resolve(arg, full);
                        if (!ramfs_exists(full) || !ramfs_is_dir(full)) {
                            vga_print("Not a directory\n");
                            return;
                        }
                        int flen = strlen(full);
                        for (int i = 0; i < ramfs_count(); i++) {
                            const char *name = ramfs_name_at(i);
                            if (!name) continue;
                            if (strncmp(name, full, flen) == 0 && name[flen] == '/') {
                                vga_print("Directory not empty\n");
                                return;
                            }
                        }
                        ramfs_delete(full);
                        vga_print("Removed: ");
                        vga_print(full);
                        vga_print("\n");
                    }

                    /* ==================== КОМАНДЫ: SOV / SP ==================== */

                    static void edit_ext_file(const char *arg, const char *ext) {
                        if (!arg || !*arg) {
                            vga_print("usage: ");
                            vga_print(ext + 1);   /* "sov" или "sp" */
                            vga_print(" NAME\n");
                            return;
                        }

                        char named[RAMFS_MAX_NAME];
                        add_ext(arg, ext, named, RAMFS_MAX_NAME);

                        char full[RAMFS_PATH_MAX];
                        ramfs_resolve(named, full);

                        if (ramfs_exists(full) && ramfs_is_dir(full)) {
                            vga_print("Is a directory\n");
                            return;
                        }

                        if (!ramfs_exists(full)) {
                            if (ramfs_create(full, RAMFS_TYPE_FILE) != 0) {
                                vga_print("Cannot create file\n");
                                return;
                            }
                        }

                        vga_print("Editing ");
                        vga_print(full);
                        vga_print(". End with '.' on empty line:\n");

                        char buf[RAMFS_MAX_FILE_SIZE];
                        uint32_t start = 0;
                        uint32_t sz = 0;
                        const char *old = ramfs_read(full, &sz);
                        if (old && sz > 0) {
                            if (sz > RAMFS_MAX_FILE_SIZE) sz = RAMFS_MAX_FILE_SIZE;
                            memcpy(buf, old, sz);
                            start = sz;
                        }

                        int added = read_multiline(buf + start,
                                                   RAMFS_MAX_FILE_SIZE - start, 0);
                        ramfs_write(full, buf, start + (uint32_t)added);

                        vga_print("Saved ");
                        vga_print_dec(start + (uint32_t)added);
                        vga_print(" bytes to ");
                        vga_print(full);
                        vga_print("\n");
                    }

                    static void cmd_sov(const char *arg) { edit_ext_file(arg, ".sov"); }
                    static void cmd_sp (const char *arg) { edit_ext_file(arg, ".sp");  }

                    /* ==================== КОМАНДЫ: ATA ==================== */

                    static void cmd_ata_info(void) {
                        if (ata_init() != 0) {
                            vga_print("No ATA disk detected\n");
                            vga_print("Tip: launch QEMU with -drive file=disk.img,format=raw,if=ide\n");
                            return;
                        }
                        vga_print("Model: ");
                        vga_print(ata_model());
                        vga_print("\nSectors: ");
                        vga_print_dec(ata_sectors());
                        vga_print("  (");
                        vga_print_dec(ata_sectors() / 2048);
                        vga_print(" MB)\n");
                    }

                    static void cmd_ata_read(const char *arg) {
                        if (!arg || !*arg) { vga_print("usage: ata-read LBA\n"); return; }
                        uint32_t lba = 0;
                        while (*arg >= '0' && *arg <= '9') { lba = lba * 10 + (*arg - '0'); arg++; }

                        static uint8_t sector[ATA_SECTOR_SIZE];
                        if (ata_read_sectors(lba, 1, sector) != 0) {
                            vga_print("Read failed\n");
                            return;
                        }
                        vga_print("Sector ");
                        vga_print_dec(lba);
                        vga_print(":\n");
                        hexdump(sector, 128);
                    }

                    static void cmd_ata_write(const char *arg) {
                        if (!arg || !*arg) { vga_print("usage: ata-write LBA\n"); return; }
                        uint32_t lba = 0;
                        while (*arg >= '0' && *arg <= '9') { lba = lba * 10 + (*arg - '0'); arg++; }

                        static uint8_t sector[ATA_SECTOR_SIZE];
                        if (ata_read_sectors(lba, 1, sector) != 0) {
                            vga_print("Read failed (disk?)\n");
                            return;
                        }

                        static char text[ATA_SECTOR_SIZE];
                        vga_print("Enter text for sector ");
                        vga_print_dec(lba);
                        vga_print(". End with '.' on empty line:\n");
                        int len = read_multiline(text, ATA_SECTOR_SIZE, 1);

                        memcpy(sector, text, (uint32_t)len);
                        for (uint32_t i = len; i < ATA_SECTOR_SIZE; i++) sector[i] = 0;

                        if (ata_write_sectors(lba, 1, sector) != 0) {
                            vga_print("Write failed\n");
                            return;
                        }
                        vga_print("Wrote ");
                        vga_print_dec((uint32_t)len);
                        vga_print(" bytes to sector ");
                        vga_print_dec(lba);
                        vga_print("\n");
                    }

                    /* ==================== ПРОЧЕЕ ==================== */

                    static void cmd_reboot(void) {
                        __asm__ volatile ("outb %0, %1" :: "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64));
                        while (1) __asm__ volatile ("hlt");
                    }

                    /* ==================== ДИСПЕТЧЕР ==================== */

                    static void dispatch(char *cmd) {
                        char *arg = cmd;
                        while (*arg && *arg != ' ') arg++;
                        if (*arg == ' ') { *arg = 0; arg++; while (*arg == ' ') arg++; }

                        if (strcmp(cmd, "help") == 0)           cmd_help();
                        else if (strcmp(cmd, "about") == 0)     cmd_about();
                        else if (strcmp(cmd, "clear") == 0)     vga_clear();
                        else if (strcmp(cmd, "reboot") == 0)    cmd_reboot();
                        else if (strcmp(cmd, "mem") == 0)       cmd_mem();
                        else if (strcmp(cmd, "kmalloc") == 0)   cmd_kmalloc(arg);
                        else if (strcmp(cmd, "ls") == 0)        cmd_ls();
                        else if (strcmp(cmd, "cat") == 0)       cmd_cat(arg);
                        else if (strcmp(cmd, "touch") == 0)     cmd_touch(arg);
                        else if (strcmp(cmd, "write") == 0)     cmd_write(arg);
                        else if (strcmp(cmd, "append") == 0)    cmd_append(arg);
                        else if (strcmp(cmd, "edit") == 0)      cmd_edit(arg);
                        else if (strcmp(cmd, "rm") == 0)        cmd_rm(arg);
                        else if (strcmp(cmd, "pwd") == 0)       cmd_pwd();
                        else if (strcmp(cmd, "cd") == 0)        cmd_cd(arg);
                        else if (strcmp(cmd, "mkdir") == 0)     cmd_mkdir(arg);
                        else if (strcmp(cmd, "rmdir") == 0)     cmd_rmdir(arg);
                        else if (strcmp(cmd, "sov") == 0)       cmd_sov(arg);
                        else if (strcmp(cmd, "sp") == 0)        cmd_sp(arg);
                        else if (strcmp(cmd, "ata-info") == 0)  cmd_ata_info();
                        else if (strcmp(cmd, "ata-read") == 0)  cmd_ata_read(arg);
                        else if (strcmp(cmd, "ata-write") == 0) cmd_ata_write(arg);
                        else if (cmd[0] == 0) { /* пусто */ }
                        else {
                            vga_print("Unknown command: ");
                            vga_print(cmd);
                            vga_print("\n");
                        }
                    }

                    void shell_run(void) {
                        vga_print("sovetnikOS shell. Type 'help'.\n");
                        while (1) {
                            vga_print("> ");
                            read_line();
                            dispatch(line);
                        }
                    }
