# sovetnikOS 0.2

Минимальная учебная ОС для x86 (32-bit protected mode), собирается и
запускается под Linux.

## Что внутри

- NASM bootloader с Multiboot-заголовком
- C kernel (`kmain`)
- Загрузка через GRUB (`grub-mkrescue` + `xorriso`)
- VGA text console (80x25, буфер 0xB8000)
- PS/2 keyboard polling
- Парсинг карты памяти из Multiboot (`mmap`)
- Bump allocator (`kmalloc`) + heap сразу за ядром
- initrd в формате **ustar**, загружается GRUB как модуль
- Командная оболочка

## Требования (Ubuntu / Debian / Linux Mint)

```bash
sudo apt update
sudo apt install nasm gcc binutils make grub-pc-bin grub-common xorriso qemu-system-x86
```

## Сборка

```bash
make
```

Собираются:

- `build/kernel.bin` — ядро
- `build/initrd.tar` — образ initrd (ustar) из папки `initrd/`
- `build/sovetnikOS.iso` — загрузочный ISO

## Запуск

```bash
make run
```

или вручную:

```bash
qemu-system-i386 -cdrom build/sovetnikOS.iso
```

Для выхода из QEMU: `Ctrl+Alt+G` (release mouse), затем закройте окно.

## Команды sovetnikOS

| Команда        | Описание                                        |
|----------------|-------------------------------------------------|
| `help`         | список команд                                   |
| `about`        | информация о системе                            |
| `clear`        | очистить экран                                  |
| `mem`          | объём RAM и состояние heap                      |
| `kmalloc N`    | выделить N байт и показать адрес                |
| `ls`           | список файлов из initrd                         |
| `cat NAME`     | вывести содержимое файла из initrd              |
| `reboot`       | перезагрузка через порт 0x64                    |

## Структура проекта

```
boot/         NASM + Multiboot заголовок
kernel/       ядро: kmain, shell, vga, keyboard, mem, string
fs/           парсер tar (initrd)
include/      заголовки
initrd/       файлы, попадающие в initrd.tar
build/        сгенерированные артефакты (не коммитить)
linker.ld     карта линковки ядра
grub.cfg      конфиг GRUB
Makefile      сборка
```

## Как работает память

1. GRUB передаёт в `kmain` Multiboot-структуру.
2. Ядро читает `mmap` (флаг `1 << 6`) и суммирует доступную RAM.
3. Heap начинается сразу за ядром (символ `_kernel_end` из `linker.ld`)
   и заканчивается за страницу до верхней границы RAM.
4. `kmalloc(size)` — простой bump allocator: двигает указатель вперёд
   с выравниванием по 4 байта. Освобождения памяти нет (учебная ОС).

## Как работает файловая система

1. В `grub.cfg` указан `module /boot/initrd.tar`.
2. GRUB загружает `initrd.tar` в память и передаёт его адрес в
   `mbi->mods_addr` (флаг `1 << 3`).
3. Ядро вызывает `tar_init(addr)` — парсер формата **ustar**:
   заголовок 512 байт, содержимое выровнено до 512.
4. Команды `ls` и `cat` читают файлы из распарсенного списка.

Важно: архив собирается именно в формате `ustar`
(`tar --format=ustar`), а не `gnu` — иначе парсер не поймёт.

## Добавить файл в initrd

```bash
echo "мой текст" > initrd/myfile.txt
make
```

Файлы кладутся плоско (без подпапок) — так проще парсеру.

## Очистка

```bash
make clean
```

## Идеи для развития

- IDT + PIC + IRQ (прерывания вместо polling клавиатуры)
- PIT-таймер и системное время
- Paging (страничная адресация)
- Heap с `kfree` (free-list, buddy allocator)
- FAT12/16 на RAM-диске вместо tar
- Переход в ring 3 и системные вызовы

## Лицензия

См. `LICENSE.txt`.
