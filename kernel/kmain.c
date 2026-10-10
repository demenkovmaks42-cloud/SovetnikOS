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

void irq0_handler(void) { pit_tick(); pic_eoi(0); }
void irq1_handler(void) { pic_eoi(1); }
void irq2_handler(void) { pic_eoi(2); }
void irq3_handler(void) { pic_eoi(3); }
void irq4_handler(void) { pic_eoi(4); }
void irq5_handler(void) { pic_eoi(5); }
void irq6_handler(void) { pic_eoi(6); }
void irq7_handler(void) { pic_eoi(7); }
void irq8_handler(void) { pic_eoi(8); }
void irq9_handler(void) { pic_eoi(9); }
void irq10_handler(void) { pic_eoi(10); }
void irq11_handler(void) { pic_eoi(11); }
void irq12_handler(void) { pic_eoi(12); }
void irq13_handler(void) { pic_eoi(13); }
void irq14_handler(void) { pic_eoi(14); }
void irq15_handler(void) { pic_eoi(15); }

static uint32_t saturating_add_u32(uint32_t a, uint32_t b)
{
    if (a > 0xFFFFFFFFu - b) return 0xFFFFFFFFu;
    return a + b;
}

static uint32_t clamp_u64_to_u32(uint64_t value)
{
    return value > 0xFFFFFFFFull ? 0xFFFFFFFFu : (uint32_t)value;
}

static void halt_forever(void)
{
    for (;;) __asm__ volatile ("cli; hlt");
}

static int find_heap_region(multiboot_info_t *mbi, uint32_t floor,
                            uint32_t *region_start, uint32_t *region_end)
{
    uint32_t addr, remaining;
    uint32_t nearest_start = 0, nearest_end = 0;
    uint32_t containing_end = 0;
    if (!(mbi->flags & MULTIBOOT_INFO_MMAP)) return 0;
    addr = mbi->mmap_addr;
    remaining = mbi->mmap_length;
    while (remaining >= 24u) {
        multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)addr;
        uint32_t step;
        if (entry->size < 20u) break;
        step = entry->size + sizeof(uint32_t);
        if (step > remaining || step < 24u) break;
        if (entry->type == 1u) {
            uint32_t start = clamp_u64_to_u32(entry->addr);
            uint32_t end = clamp_u64_to_u32(entry->addr + entry->len);
            if (end > start) {
                if (start <= floor && floor < end) {
                    containing_end = end;
                    *region_start = floor;
                } else if (start >= floor && (!nearest_start || start < nearest_start)) {
                    nearest_start = start;
                    nearest_end = end;
                }
            }
        }
        addr += step;
        remaining -= step;
    }
    if (containing_end) {
        *region_end = containing_end;
        return 1;
    }
    if (nearest_start) {
        *region_start = nearest_start;
        *region_end = nearest_end;
        return 1;
    }
    return 0;
}

void kmain(uint32_t magic, multiboot_info_t *mbi)
{
    uint32_t total_ram = 0;
    uint32_t max_addr = 0;
    uint32_t heap_start;
    uint32_t heap_end;

    /* Initialize a text console before printing any diagnostic. */
    vga_init();
    if (magic != 0x2BADB002u || mbi == NULL) {
        vga_print("sovetnikOS: invalid Multiboot handoff\n");
        halt_forever();
    }

    if ((mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER) &&
        mbi->framebuffer_type == 1 &&
        mbi->framebuffer_addr_high == 0 &&
        mbi->framebuffer_addr_low != 0 &&
        mbi->framebuffer_width != 0 && mbi->framebuffer_height != 0) {
        vga_init_fb(mbi->framebuffer_addr_low,
                    mbi->framebuffer_pitch,
                    mbi->framebuffer_width,
                    mbi->framebuffer_height,
                    mbi->framebuffer_bpp);
    }

    vga_print("sovetnikOS 0.7 booting...\n");

    gdt_init();
    keyboard_init();

    /* Read the Multiboot memory map with entry-size checks. */
    if (mbi->flags & MULTIBOOT_INFO_MMAP) {
        uint32_t addr = mbi->mmap_addr;
        uint32_t remaining = mbi->mmap_length;
        while (remaining >= 24u) {
            multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)addr;
            uint32_t step;
            if (entry->size < 20u) break;
            step = entry->size + sizeof(uint32_t);
            if (step > remaining || step < 24u) break;
            if (entry->type == 1u) {
                uint32_t len_low = (uint32_t)entry->len;
                uint32_t end_addr = clamp_u64_to_u32(entry->addr + entry->len);
                total_ram = saturating_add_u32(total_ram, len_low);
                if (end_addr > max_addr) max_addr = end_addr;
            }
            addr += step;
            remaining -= step;
        }
    } else if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
        total_ram = clamp_u64_to_u32(((uint64_t)mbi->mem_upper + 1024u) * 1024u);
        max_addr = total_ram;
    }
    mem_set_total_ram(total_ram);

    /* Find a contiguous usable RAM region after the kernel and boot modules. */
    heap_start = (uint32_t)&_kernel_end;
    if ((uint32_t)mbi <= 0xFFFFFFFFu - (uint32_t)sizeof(*mbi)) {
        uint32_t mbi_end = (uint32_t)mbi + (uint32_t)sizeof(*mbi);
        if (mbi_end > heap_start) heap_start = mbi_end;
    }
    if (mbi->flags & MULTIBOOT_INFO_MODS) {
        uint32_t i;
        multiboot_module_t *mods = (multiboot_module_t *)mbi->mods_addr;
        if (mbi->mods_count <= 1024u && mbi->mods_addr <= 0xFFFFFFFFu - mbi->mods_count * (uint32_t)sizeof(multiboot_module_t)) {
            uint32_t mods_end = mbi->mods_addr + mbi->mods_count * (uint32_t)sizeof(multiboot_module_t);
            if (mods_end > heap_start) heap_start = mods_end;
        }
        for (i = 0; i < mbi->mods_count && i < 1024u; ++i) {
            if (mods[i].mod_end > heap_start) heap_start = mods[i].mod_end;
        }
    }
    if ((mbi->flags & MULTIBOOT_INFO_MMAP) && mbi->mmap_addr <= 0xFFFFFFFFu - mbi->mmap_length) {
        uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
        if (mmap_end > heap_start) heap_start = mmap_end;
    }

    if (mbi->flags & MULTIBOOT_INFO_MMAP) {
        uint32_t region_start = 0, region_end = 0;
        if (find_heap_region(mbi, heap_start, &region_start, &region_end)) {
            heap_start = region_start;
            heap_end = region_end > 0x1000u ? region_end - 0x1000u : 0;
        } else {
            heap_start = 0;
            heap_end = 0;
        }
    } else {
        heap_end = max_addr > 0x1000u ? max_addr - 0x1000u : 0;
    }

    if (heap_start <= 0xFFFFFFFFu - 7u) heap_start = (heap_start + 7u) & ~7u;
    else heap_start = 0;
    if (heap_start && heap_end > heap_start && heap_end - heap_start >= 0x10000u) {
        mem_init(heap_start, heap_end);
    } else {
        mem_init(0, 0);
        heap_start = 0;
        vga_print("Warning: heap disabled (no safe contiguous RAM range).\n");
    }

    vga_print("RAM detected: ");
    vga_print_dec(total_ram / 1024u / 1024u);
    vga_print(" MB\n");
    if (heap_start) {
        vga_print("Heap: "); vga_print_hex(mem_heap_start);
        vga_print(" - "); vga_print_hex(mem_heap_end); vga_putc('\n');
    }

    ramfs_init();
    if ((mbi->flags & MULTIBOOT_INFO_MODS) && mbi->mods_count > 0) {
        multiboot_module_t *mods = (multiboot_module_t *)mbi->mods_addr;
        if (mods[0].mod_start && mods[0].mod_end > mods[0].mod_start) {
            ramfs_load_initrd((void *)mods[0].mod_start);
            vga_print("initrd loaded; RAMFS entries: ");
            vga_print_dec((uint32_t)ramfs_count());
            vga_putc('\n');
        } else {
            vga_print("initrd module is invalid\n");
        }
    } else {
        vga_print("initrd not found\n");
    }

    if (ata_init() == 0) {
        vga_print("ATA: "); vga_print(ata_model());
        vga_print(" (sectors: "); vga_print_dec(ata_sectors());
        vga_print(")\n");
    } else {
        vga_print("ATA: no primary-master disk\n");
    }

    idt_init();
    pic_init();
    pit_init(100);
    __asm__ volatile ("sti");

    shell_run();
    halt_forever();
}
