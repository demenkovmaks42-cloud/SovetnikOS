; Multiboot + точка входа (x86, 32-bit)
BITS 32

section .multiboot
align 4                     ; Обязательное выравнивание по 4 байта
    dd 0x1BADB002           ; magic
    dd 0x00000007           ; flags: ALIGN + MEMINFO + VIDEO_MODE
    dd -(0x1BADB002 + 0x00000007) ; checksum

    ; AOUT kludge (эти пять dword-ов обязательны для работы видео-полей)
    dd 0                    ; header_addr (игнорируется, если не установлен флаг 16)
    dd 0                    ; load_addr
    dd 0                    ; load_end_addr
    dd 0                    ; bss_end_addr
    dd 0                    ; entry_addr

    ; Теперь идут поля видео-режима
    dd 0                    ; mode_type: 0 = linear graphics
    dd 1024                 ; width
    dd 768                  ; height
    dd 32                   ; depth

section .bss
align 16
stack_bottom:
    resb 16384                          ; 16 KB стек
stack_top:

section .text
global _start
extern kmain

_start:
    cli
    mov esp, stack_top
    push ebx                            ; mbi
    push eax                            ; magic
    call kmain
.hang:
    hlt
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits
