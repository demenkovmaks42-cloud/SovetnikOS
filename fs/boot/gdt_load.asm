BITS 32

section .text
global gdt_load
gdt_load:
    mov eax, [esp + 4]      ; указатель на gdt_ptr
    lgdt [eax]

    ; перезагрузить сегментные регистры
    mov ax, 0x10            ; data segment = 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; дальний jump для cs
    jmp 0x08:.reload
.reload:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
