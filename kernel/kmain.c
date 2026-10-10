#include "types.h"
#include "multiboot.h"
#include "vga.h"
#include "keyboard.h"
#include "mem.h"
#include "tar.h"
#include "ramfs.h"
#include "ata.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "shell.h"

extern uint32_t _kernel_end;
void mem_set_total_ram(uint32_t bytes);

/* ==================== IRQ HANDLERS ==================== */
/* Вызываются из boot/isr_stubs.asm через irqN_stub */

void irq0_handler(void) {   /* таймер PIT */
    pit_tick();
    pic_eoi(0);
}

void irq1_handler(void) {   /* клавиатура — пока polling, просто EOI */
    pic_eoi(1);
}

void irq2_handler(void)  { pic_eoi(2);  }
void irq3_handler(void)  { pic_eoi(3);  }
void irq4_handler(void)  { pic_eoi(4);  }
void irq5_handler(void)  { pic_eoi(5);  }
void irq6_handler(void)  { pic_eoi(6);  }
void irq7_handler(void)  { pic_eoi(7);  }
void irq8_handler(void)  { pic_eoi(8);  }
void irq9_handler(void)  { pic_eoi(9);  }
void irq10_handler(void) { pic_eoi(10); }
void irq11_handler(void) { pic_eoi(11); }
void irq12_handler(void) { pic_eoi(12); }
void irq13_handler(void) { pic_eoi(13); }
void irq14_handler(void) { pic_eoi(14); }
void irq15_handler(void) { pic_eoi(15); }

/* ==================== KMAIN ==================== */

void kmain(uint32_t magic, multiboot_info_t *mbi) {
    /* 1. VGA — самый первый, чтобы видеть ошибки */
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
            vga_print("VGA text mode fallback\n");
        }

        /* 2. GDT — обязательно ПЕРВОЙ среди системных */
        gdt_init();

        /* 3. Клавиатура */
        keyboard_init();

        vga_print("sovetnikOS 0.6 booting...\n");

        if (magic != 0x2BADB002) {
            vga_print("Bad multiboot magic\n");
            while (1) __asm__ volatile ("hlt");
        }

        /* 4. Парсинг mmap */
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

        /* 5. Heap */
        uint32_t heap_start = (uint32_t)&_kernel_end;
        uint32_t heap_end   = max_addr - 0x1000;
        if (heap_end < heap_start + 0x10000) heap_end = heap_start + 0x10000;
        mem_init(heap_start, heap_end);

    vga_print("RAM: ");
    vga_print_dec(total_ram / 1024 / 1024);
    vga_print(" MB, heap at ");
    vga_print_hex(heap_start);
    vga_print("\n");

    /* 6. ramfs + initrd */
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

    /* 7. ATA */
    if (ata_init() == 0) {
        vga_print("ATA: ");
        vga_print(ata_model());
        vga_print(", ");
        vga_print_dec(ata_sectors() / 2048);
        vga_print(" MB\n");
    } else {
        vga_print("ATA: no disk\n");
    }

    /* 8. IDT + PIC + PIT
     *    ВАЖНО: gdt_init() уже вызвана выше.
     *    Порядок: idt_init → pic_init → pit_init → sti */
    idt_init();
    pic_init();
    pit_init(100);   /* 100 Гц */

    __asm__ volatile ("sti");   /* включаем прерывания */

    /* 9. Shell */
    shell_run();

    while (1) __asm__ volatile ("hlt");
}
