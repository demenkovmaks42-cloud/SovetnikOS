BITS 32

section .text
global idt_load
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

%macro IRQ_STUB 2
global %1
extern %2
%1:
    pusha
    call %2
    popa
    iret
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

section .note.GNU-stack noalloc noexec nowrite progbits
