#include "types.h"
#include "multiboot.h"
#include "vga.h"
#include "keyboard.h"
#include "mem.h"
#include "tar.h"
#include "shell.h"

extern uint32_t _kernel_end;

void mem_set_total_ram(uint32_t bytes);

void kmain(uint32_t magic, multiboot_info_t *mbi) {
    vga_init();
    keyboard_init();

    vga_print("sovetnikOS 0.2 booting...\n");

    if (magic != 0x2BADB002) {
        vga_print("Bad multiboot magic\n");
        while (1) __asm__ volatile ("hlt");
    }

    uint32_t total = 0;
    uint32_t max_addr = 0;
    if (mbi->flags & MULTIBOOT_INFO_MMAP) {
        uint32_t addr = mbi->mmap_addr;
        uint32_t end  = addr + mbi->mmap_length;
        while (addr < end) {
            multiboot_mmap_entry_t *e = (multiboot_mmap_entry_t*)addr;
            if (e->type == 1) {
                total += (uint32_t)e->len;
                uint32_t top = (uint32_t)(e->addr + e->len);
                if (top > max_addr) max_addr = top;
            }
            addr += e->size + sizeof(uint32_t);
        }
    } else {
        total = (mbi->mem_upper + 1024) * 1024;
        max_addr = total;
    }
    mem_set_total_ram(total);

    uint32_t heap_start = (uint32_t)&_kernel_end;
    uint32_t heap_end   = max_addr - 0x1000;
    if (heap_end < heap_start + 0x10000) heap_end = heap_start + 0x10000;
    mem_init(heap_start, heap_end);

    vga_print("RAM: ");
    vga_print_dec(total / 1024 / 1024);
    vga_print(" MB, heap at ");
    vga_print_hex(heap_start);
    vga_print("\n");

    if ((mbi->flags & MULTIBOOT_INFO_MODS) && mbi->mods_count > 0) {
        multiboot_module_t *mods = (multiboot_module_t*)mbi->mods_addr;
        tar_init((void*)mods[0].mod_start);
        vga_print("initrd loaded: ");
        vga_print_dec(tar_count());
        vga_print(" files\n");
    } else {
        vga_print("initrd not found\n");
    }

    shell_run();

    while (1) __asm__ volatile ("hlt");
}
