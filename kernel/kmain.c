#include "types.h"
#include "multiboot.h"
#include "vga.h"
#include "keyboard.h"
#include "mem.h"
#include "ramfs.h"
#include "ata.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "shell.h"

/* Символ задаётся в linker.ld */
extern uint32_t _kernel_end;

/* Реализована в kernel/mem.c */
extern void mem_set_total_ram(uint32_t bytes);

/* ================= INTERRUPT HANDLERS ================= */

void irq0_handler(void)
{
    pit_tick();
    pic_eoi(0);
}

/*
 * Клавиатура опрашивается через polling.
 * IRQ1 оставляем замаскированным, чтобы обработчик
 * не получал данные клавиатуры, которые не читает.
 */
void irq1_handler(void)
{
    pic_eoi(1);
}

void irq2_handler(void)  { pic_eoi(2); }
void irq3_handler(void)  { pic_eoi(3); }
void irq4_handler(void)  { pic_eoi(4); }
void irq5_handler(void)  { pic_eoi(5); }
void irq6_handler(void)  { pic_eoi(6); }
void irq7_handler(void)  { pic_eoi(7); }
void irq8_handler(void)  { pic_eoi(8); }
void irq9_handler(void)  { pic_eoi(9); }
void irq10_handler(void) { pic_eoi(10); }
void irq11_handler(void) { pic_eoi(11); }
void irq12_handler(void) { pic_eoi(12); }
void irq13_handler(void) { pic_eoi(13); }
void irq14_handler(void) { pic_eoi(14); }
void irq15_handler(void) { pic_eoi(15); }

/* ================= MEMORY DETECTION ================= */

static uint32_t detect_total_ram(multiboot_info_t *mbi)
{
    uint32_t total = 0;

    if (!mbi)
        return 0;

    if (mbi->flags & MULTIBOOT_INFO_MMAP) {
        uint32_t current = mbi->mmap_addr;
        uint32_t end = mbi->mmap_addr + mbi->mmap_length;

        while (current < end) {
            multiboot_mmap_entry_t *entry =
            (multiboot_mmap_entry_t *)current;

            uint32_t entry_size = entry->size + sizeof(uint32_t);

            if (entry_size < sizeof(multiboot_mmap_entry_t))
                break;

            if (current + entry_size < current)
                break;

            if (current + entry_size > end)
                break;

            if (entry->type == 1) {
                uint64_t length = entry->len;

                if (length > 0xFFFFFFFFULL - total)
                    total = 0xFFFFFFFFu;
                else
                    total += (uint32_t)length;
            }

            current += entry_size;
        }
    } else if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
        total = (mbi->mem_upper + 1024u) * 1024u;
    }

    return total;
}

/*
 * Для простой модели памяти выбираем конец heap
 * ниже обнаруженного верхнего адреса.
 */
static uint32_t detect_heap_end(multiboot_info_t *mbi)
{
    uint32_t max_address = 0;

    if (mbi && (mbi->flags & MULTIBOOT_INFO_MMAP)) {
        uint32_t current = mbi->mmap_addr;
        uint32_t end = mbi->mmap_addr + mbi->mmap_length;

        while (current < end) {
            multiboot_mmap_entry_t *entry =
            (multiboot_mmap_entry_t *)current;

            uint32_t entry_size = entry->size + sizeof(uint32_t);

            if (entry_size < sizeof(multiboot_mmap_entry_t))
                break;

            if (current + entry_size < current ||
                current + entry_size > end)
                break;

            if (entry->type == 1) {
                uint64_t top64 = entry->addr + entry->len;

                if (top64 <= 0xFFFFFFFFULL) {
                    uint32_t top = (uint32_t)top64;

                    if (top > max_address)
                        max_address = top;
                }
            }

            current += entry_size;
        }
    }

    /*
     * Для стандартной конфигурации QEMU без mmap
     * используем информацию Multiboot о памяти.
     */
    if (max_address == 0 && mbi &&
        (mbi->flags & MULTIBOOT_INFO_MEMORY)) {
        max_address = (mbi->mem_upper + 1024u) * 1024u;
        }

        /*
         * Оставляем запас внизу верхней границы.
         */
        if (max_address > 0x1000u)
            max_address -= 0x1000u;

    return max_address;
}

/* ================= KERNEL ENTRY ================= */

void kmain(uint32_t magic, multiboot_info_t *mbi)
{
    uint32_t total_ram;
    uint32_t heap_start = 0;
    uint32_t heap_end = 0;

    /* 1. Проверяем способ загрузки */
    if (magic != 0x2BADB002u || mbi == NULL) {
        vga_init();
        vga_print("SovetnikOS: invalid Multiboot information.\n");

        for (;;)
            __asm__ volatile ("cli; hlt");
    }

    /* 2. Инициализируем экран */
    if ((mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER) &&
        mbi->framebuffer_type == 1 &&
        mbi->framebuffer_addr_low != 0 &&
        mbi->framebuffer_width > 0 &&
        mbi->framebuffer_height > 0) {

        vga_init_fb(
            mbi->framebuffer_addr_low,
            mbi->framebuffer_pitch,
            mbi->framebuffer_width,
            mbi->framebuffer_height,
            mbi->framebuffer_bpp
        );
        } else {
            vga_init();
        }

        vga_clear();

        vga_set_fg(0x0055FF55);
        vga_print("================================\n");
        vga_print("       SovetnikOS 0.6\n");
        vga_print("================================\n");
        vga_set_fg(0x00C0C0C0);

        /* 3. Сегментные регистры и GDT */
        gdt_init();
        vga_print("[OK] GDT initialized\n");

        /* 4. Инициализируем клавиатуру */
        keyboard_init();
        vga_print("[OK] Keyboard initialized\n");

        /* 5. Определяем память */
        total_ram = detect_total_ram(mbi);
        mem_set_total_ram(total_ram);
        mem_init(heap_start, heap_end);

        heap_start = ((uint32_t)&_kernel_end + 0xFFFu) & ~0xFFFu;
        heap_end = detect_heap_end(mbi);

        if (heap_end <= heap_start) {
            vga_print("[ERROR] Not enough memory for heap.\n");

            for (;;)
                __asm__ volatile ("cli; hlt");
        }

        mem_init(heap_start, heap_end);

        vga_print("[OK] RAM detected: ");
        vga_print_dec(total_ram / (1024u * 1024u));
        vga_print(" MiB\n");

        vga_print("[OK] Heap start: ");
        vga_print_hex(heap_start);
        vga_putc('\n');

        /* 6. RAM filesystem */
        ramfs_init();
        vga_print("[OK] RAMFS initialized\n");

        /* 7. Загружаем initrd, если GRUB передал модуль */
        if ((mbi->flags & MULTIBOOT_INFO_MODS) &&
            mbi->mods_count > 0 &&
            mbi->mods_addr != 0) {

            multiboot_module_t *modules =
            (multiboot_module_t *)mbi->mods_addr;

        if (modules[0].mod_start != 0 &&
            modules[0].mod_end > modules[0].mod_start) {

            ramfs_load_initrd((void *)modules[0].mod_start);

        vga_print("[OK] initrd loaded; RAMFS objects: ");
        vga_print_dec(ramfs_count());
        vga_putc('\n');
            } else {
                vga_print("[WARN] Invalid initrd module\n");
            }
            } else {
                vga_print("[WARN] initrd not found\n");
            }

            /* 8. Инициализируем ATA-диск */
            if (ata_init() == 0) {
                vga_print("[OK] ATA: ");
                vga_print(ata_model());
                vga_print("\n");

                vga_print("     Sectors: ");
                vga_print_dec(ata_sectors());
                vga_putc('\n');
            } else {
                vga_print("[WARN] ATA disk not detected\n");
            }

            /*
             * 9. Таблица прерываний, PIC и таймер.
             * Не включаем прерывания до готовности IDT и PIT.
             */
            __asm__ volatile ("cli");

            idt_init();
            pic_init();
            pit_init(100);

            __asm__ volatile ("sti");

            vga_print("[OK] IDT initialized\n");
            vga_print("[OK] PIC initialized\n");
            vga_print("[OK] PIT initialized at 100 Hz\n");

            /* 10. Запускаем командную оболочку */
            shell_run();

            /* shell_run() должен работать бесконечно */
            for (;;)
                __asm__ volatile ("hlt");
}
