AS = nasm
CC = gcc
LD = ld

CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -nostdinc -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

BUILD = build

OBJECTS = \
	$(BUILD)/boot.o \
	$(BUILD)/kernel.o \
	$(BUILD)/keyboard.o \
	$(BUILD)/mouse.o \
	$(BUILD)/gui.o \
	$(BUILD)/console.o

all: $(BUILD)/sovetnikOS.iso

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/boot.o: boot/boot.asm | $(BUILD)
	$(AS) -f elf32 boot/boot.asm -o $@

$(BUILD)/kernel.o: kernel/kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c kernel/kernel.c -o $@

$(BUILD)/keyboard.o: drivers/keyboard.c | $(BUILD)
	$(CC) $(CFLAGS) -c drivers/keyboard.c -o $@

$(BUILD)/mouse.o: drivers/mouse.c | $(BUILD)
	$(CC) $(CFLAGS) -c drivers/mouse.c -o $@

$(BUILD)/gui.o: gui/gui.c | $(BUILD)
	$(CC) $(CFLAGS) -c gui/gui.c -o $@

$(BUILD)/console.o: kernel/console.c | $(BUILD)
	$(CC) $(CFLAGS) -c kernel/console.c -o $@

$(BUILD)/kernel.bin: $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(BUILD)/iso:
	mkdir -p $(BUILD)/iso/boot/grub

$(BUILD)/sovetnikOS.iso: $(BUILD)/kernel.bin $(BUILD)/iso
	cp $(BUILD)/kernel.bin $(BUILD)/iso/boot/kernel.bin
	cp boot/grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso

run: $(BUILD)/sovetnikOS.iso
	qemu-system-i386 -cdrom $(BUILD)/sovetnikOS.iso

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
