# sovetnikOS 0.1

Минимальная учебная ОС x86 для Linux:
- NASM bootloader
- C kernel
- GRUB Multiboot
- VGA text console
- PS/2 keyboard polling
- PS/2 mouse polling
- простая текстовая GUI-заставка
- командная оболочка

## Требования (Ubuntu/Debian/Linux Mint)

    sudo apt update
    sudo apt install nasm gcc binutils make grub-pc-bin grub-common xorriso qemu-system-x86

## Сборка

    make

## Запуск

    make run

или:

    qemu-system-i386 -cdrom build/sovetnikOS.iso

## Очистка

    make clean

## Команды sovetnikOS

    help
    about
    clear
    gui
    mem
    reboot

Для выхода из QEMU обычно используйте Ctrl+Alt+G (release mouse) и закройте окно.
