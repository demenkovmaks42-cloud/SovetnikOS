#!/bin/sh
set -e
command -v nasm
command -v gcc
command -v ld
command -v grub-mkrescue
command -v qemu-system-i386
echo "All required commands found."
