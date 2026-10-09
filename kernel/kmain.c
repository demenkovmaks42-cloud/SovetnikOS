#include "types.h"
#include "multiboot.h"
#include "vga.h"
#include "keyboard.h"
#include "mem.h"
#include "tar.h"
#include "ramfs.h"
#include "ata.h"
#include "shell.h"

extern uint32_t _kernel_end;
void mem_set_total_ram(uint32_t bytes);

void kmain(uint32_t magic, multiboot_info_t *mbi) {
    /* 1. Инициализация вывода.
     *    Если GRUB отдал framebuffer — используем его.
     *    Иначе — fallback в VGA text. */
    if (magic == 0x2BADB002 &&
        (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER) &&
        mbi->framebuffer_type == 1 &&
        mbi->framebuffer_addr_low != 0) {

        vga_init_fb(mbi->framebuffer_addr_low,
                    mbi->framebuffer_pitch,
                    mbi->framebuffer_width,
                    mbi->framebuffer_height,
                    mbi->framebuffer_bpp);

        vga_print("Framebuffer: ");
        vga_print_dec(mbi->framebuffer_width);
        vga_print("x");
        vga_print_dec(mbi->framebuffer_height);
        vga_print("x");
        vga_print_dec(mbi->framebuffer_bpp);
        vga_print("\n");
        } else {
            vga_init();
            vga_print("VGA text mode (80x25) fallback\n");
        }

        keyboard_init();
        vga_print("sovetnikOS 0.5 booting...\n");

        if (magic != 0x2BADB002) {
            vga_print("Bad multiboot magic\n");
            while (1) __asm__ volatile ("hlt");
        }

        /* 2. mmap */
        uint32_t total_ram = 0;
        uint32_t max_addr  = 0;
        if (mbi->flags & MULTIBOOT_INFO_MMAP) {
            uint32_t addr = mbi->mmap_addr;
            uint32_t end  = addr + mbi->mmap_length;
            while (addr < end) {
                multiboot_mmap_entry_t *e = (multiboot_mmap_entry_t*)addr;
                if (e->type == 1) {
                    total_ram += (uint32_t)e->len;
                    uint32_t top = (uint32_t)(e->addr + e->len);
                    if (top > max_addr) max_addr = top;
                }
                addr += e->size + sizeof(uint32_t);
            }
        } else {
            total_ram = (mbi->mem_upper + 1024) * 1024;
            max_addr  = total_ram;
        }
        mem_set_total_ram(total_ram);

        /* 3. heap */
        uint32_t heap_start = (uint32_t)&_kernel_end;
        uint32_t heap_end   = max_addr - 0x1000;
        if (heap_end < heap_start + 0x10000) heap_end = heap_start + 0x10000;
        mem_init(heap_start, heap_end);

    vga_print("RAM: ");
    vga_print_dec(total_ram / 1024 / 1024);
    vga_print(" MB, heap at ");
    vga_print_hex(heap_start);
    vga_print("\n");

    /* 4. ramfs + initrd */
    ramfs_init();
    if ((mbi->flags & MULTIBOOT_INFO_MODS) && mbi->mods_count > 0) {
        multiboot_module_t *mods = (multiboot_module_t*)mbi->mods_addr;
        ramfs_load_initrd((void*)mods[0].mod_start);
        vga_print("initrd loaded: ");
        vga_print_dec(ramfs_count());
        vga_print(" files\n");
    } else {
        vga_print("initrd not found\n");
    }

    /* 5. ATA */
    if (ata_init() == 0) {
        vga_print("ATA: ");
        vga_print(ata_model());
        vga_print(", ");
        vga_print_dec(ata_sectors() / 2048);
        vga_print(" MB\n");
    } else {
        vga_print("ATA: no disk\n");
    }

    /* 6. shell */
    shell_run();

    while (1) __asm__ volatile ("hlt");
}
