# =========================================================
#  sovetnikOS 0.5 — Makefile
# =========================================================

CC      := gcc
AS      := nasm
LD      := ld

CFLAGS  := -m32 -ffreestanding -nostdlib \
           -fno-stack-protector -fno-pic -fno-builtin \
           -Wall -Wextra -O2 -Iinclude

LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

BUILD   := build
KERNEL  := $(BUILD)/kernel.bin
ISO     := $(BUILD)/sovetnikOS.iso
INITRD  := $(BUILD)/initrd.tar
DISK    := disk.img

C_SRCS  := $(wildcard kernel/*.c) \
           $(wildcard fs/*.c) \
           $(wildcard drivers/*.c)

ASM_SRCS := $(wildcard boot/*.asm)

C_OBJS  := $(patsubst %.c,$(BUILD)/%.o,$(C_SRCS))
ASM_OBJS:= $(patsubst %.asm,$(BUILD)/%.o,$(ASM_SRCS))
OBJS    := $(ASM_OBJS) $(C_OBJS)

all: $(ISO)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) -f elf32 $< -o $@

$(KERNEL): $(OBJS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(INITRD): $(shell find initrd -type f 2>/dev/null)
	@mkdir -p $(BUILD)
	tar --format=ustar -cf $@ -C initrd .

$(ISO): $(KERNEL) $(INITRD) grub.cfg
	@mkdir -p $(BUILD)/iso/boot/grub
	cp $(KERNEL) $(BUILD)/iso/boot/kernel.bin
	cp $(INITRD) $(BUILD)/iso/boot/initrd.tar
	cp grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD)/iso

$(DISK):
	dd if=/dev/zero of=$(DISK) bs=1M count=16

run: $(ISO) $(DISK)
	qemu-system-i386 \
	    -cdrom $(ISO) \
	    -drive file=$(DISK),format=raw,if=ide \
	    -vga std

run-nodisk: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -vga std

clean:
	rm -rf $(BUILD)

distclean: clean
	rm -f $(DISK)

.PHONY: all run run-nodisk clean distclean
