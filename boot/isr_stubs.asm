BITS 32

section .text
global idt_load
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; IRQ stubs: preserve general-purpose registers around C handlers.
%macro IRQ_STUB 2
global %1
extern %2
%1:
    cld
    pusha
    call %2
    popa
    iretd
%endmacro

IRQ_STUB irq0_stub,  irq0_handler
IRQ_STUB irq1_stub,  irq1_handler
IRQ_STUB irq2_stub,  irq2_handler
IRQ_STUB irq3_stub,  irq3_handler
IRQ_STUB irq4_stub,  irq4_handler
IRQ_STUB irq5_stub,  irq5_handler
IRQ_STUB irq6_stub,  irq6_handler
IRQ_STUB irq7_stub,  irq7_handler
IRQ_STUB irq8_stub,  irq8_handler
IRQ_STUB irq9_stub,  irq9_handler
IRQ_STUB irq10_stub, irq10_handler
IRQ_STUB irq11_stub, irq11_handler
IRQ_STUB irq12_stub, irq12_handler
IRQ_STUB irq13_stub, irq13_handler
IRQ_STUB irq14_stub, irq14_handler
IRQ_STUB irq15_stub, irq15_handler

; Exceptions without a CPU-pushed error code.
%macro EXC_NOERR 1
global exception_stub_%1
exception_stub_%1:
    push dword 0
    push dword %1
    jmp exception_common
%endmacro

; Exceptions where the CPU already pushes an error code.
%macro EXC_ERR 1
global exception_stub_%1
exception_stub_%1:
    push dword %1
    jmp exception_common
%endmacro

EXC_NOERR 0
EXC_NOERR 1
EXC_NOERR 2
EXC_NOERR 3
EXC_NOERR 4
EXC_NOERR 5
EXC_NOERR 6
EXC_NOERR 7
EXC_ERR   8
EXC_NOERR 9
EXC_ERR   10
EXC_ERR   11
EXC_ERR   12
EXC_ERR   13
EXC_ERR   14
EXC_NOERR 15
EXC_NOERR 16
EXC_ERR   17
EXC_NOERR 18
EXC_NOERR 19
EXC_NOERR 20
EXC_ERR   21
EXC_NOERR 22
EXC_NOERR 23
EXC_NOERR 24
EXC_NOERR 25
EXC_NOERR 26
EXC_NOERR 27
EXC_NOERR 28
EXC_ERR   29
EXC_ERR   30
EXC_NOERR 31

extern exception_handler
exception_common:
    cld
    pusha
    ; At this point: vector=[esp+32], error=[esp+36], eip=[esp+40].
    mov eax, [esp + 32]
    mov edx, [esp + 36]
    mov ecx, [esp + 40]
    push ecx                ; eip
    push edx                ; error code
    push eax                ; vector
    call exception_handler
    add esp, 12
    popa
    add esp, 8              ; discard vector and error code
    iretd

section .note.GNU-stack noalloc noexec nowrite progbits
