; sovetnikOS 0.7 - Multiboot 1 entry
BITS 32

section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00000007
    dd -(0x1BADB002 + 0x00000007)

    ; AOUT fields
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0

    ; Linear graphics request
    dd 0
    dd 1024
    dd 768
    dd 32

section .bss
align 16

stack_bottom:
    resb 16384

stack_top:

section .text

global _start
extern kmain

_start:
    cli
    mov esp, stack_top
    xor ebp, ebp

    ; kmain(magic, mbi)
    push ebx
    push eax
    call kmain

.hang:
    cli
    hlt
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits
