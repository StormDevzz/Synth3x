/* Synth3x OS — IDT (Interrupt Descriptor Table) operations
 * Calling convention: cdecl (i386)
 *
 * Provides a generic interrupt service routine (ISR) stub that
 * saves all registers and calls a C handler.
 */

.section .text

/* void idt_load(uint32_t idt_ptr)
 *
 * idt_ptr points to a 6-byte pseudo-descriptor:
 *   bytes 0-1: IDT size - 1
 *   bytes 2-5: linear address of IDT
 */
.global idt_load
idt_load:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp), %eax
    lidt    (%eax)
    popl    %ebp
    ret

/* ─── ISR stubs ─────────────────────────────────────────────────── */

/* The C handler (must be provided by the kernel) */
.extern isr_handler

/* Save all registers and call the C handler */
.macro ISR_NOERRCODE n
.global isr\n
isr\n:
    cli
    pushl   $0          /* dummy error code */
    pushl   $\n         /* interrupt number */
    jmp     isr_common
.endm

.macro ISR_ERRCODE n
.global isr\n
isr\n:
    cli
    pushl   $\n         /* interrupt number (error code already on stack) */
    jmp     isr_common
.endm

/* CPU exceptions (0–31) */
ISR_NOERRCODE  0      /* Divide by Zero */
ISR_NOERRCODE  1      /* Debug */
ISR_NOERRCODE  2      /* Non-Maskable Interrupt */
ISR_NOERRCODE  3      /* Breakpoint */
ISR_NOERRCODE  4      /* Overflow */
ISR_NOERRCODE  5      /* Bound Range Exceeded */
ISR_NOERRCODE  6      /* Invalid Opcode */
ISR_NOERRCODE  7      /* Device Not Available */
ISR_ERRCODE    8      /* Double Fault */
ISR_NOERRCODE  9      /* Coprocessor Segment Overrun */
ISR_ERRCODE   10      /* Invalid TSS */
ISR_ERRCODE   11      /* Segment Not Present */
ISR_ERRCODE   12      /* Stack-Segment Fault */
ISR_ERRCODE   13      /* General Protection Fault */
ISR_ERRCODE   14      /* Page Fault */
ISR_NOERRCODE 15      /* Reserved */
ISR_NOERRCODE 16      /* x87 Floating-Point Exception */
ISR_ERRCODE   17      /* Alignment Check */
ISR_NOERRCODE 18      /* Machine Check */
ISR_NOERRCODE 19      /* SIMD Floating-Point Exception */
ISR_NOERRCODE 20      /* Virtualisation Exception */
ISR_ERRCODE   21      /* Control Protection Exception */
ISR_NOERRCODE 22      /* Reserved */
ISR_NOERRCODE 23      /* Reserved */
ISR_NOERRCODE 24      /* Reserved */
ISR_NOERRCODE 25      /* Reserved */
ISR_NOERRCODE 26      /* Reserved */
ISR_NOERRCODE 27      /* Reserved */
ISR_NOERRCODE 28      /* Reserved */
ISR_NOERRCODE 29      /* Reserved */
ISR_ERRCODE   30      /* Security Exception */
ISR_NOERRCODE 31      /* Reserved */

/* ─── IRQ stubs (32–47, remapped from PIC) ──────────────────────── */

.macro IRQ n
.global irq\n
irq\n:
    cli
    pushl   $0          /* dummy error code */
    pushl   $\n+32      /* IRQn → vector 32+n */
    jmp     irq_common
.endm

IRQ 0    /* PIT      */
IRQ 1    /* Keyboard */
IRQ 2    /* Cascade  */
IRQ 3    /* COM2     */
IRQ 4    /* COM1     */
IRQ 5    /* LPT2     */
IRQ 6    /* Floppy   */
IRQ 7    /* LPT1     */
IRQ 8    /* CMOS RTC */
IRQ 9    /* Free     */
IRQ 10   /* Free     */
IRQ 11   /* Free     */
IRQ 12   /* Mouse    */
IRQ 13   /* FPU      */
IRQ 14   /* Primary ATA */
IRQ 15   /* Secondary ATA */

/* ─── Common stub ───────────────────────────────────────────────── */

/* Stack layout at isr_common:
 *   [esp+0]  interrupt number
 *   [esp+4]  error code
 *   [esp+8]  eip
 *   [esp+12] cs
 *   [esp+16] eflags
 *   [esp+20] esp (if ring change)
 *   [esp+24] ss  (if ring change)
 *
 * We push the full GPR context to make debugging / scheduling easy.
 */

isr_common:
    pusha                     /* eax, ecx, edx, ebx, esp, ebp, esi, edi */

    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs

    movw    $0x10, %ax        /* kernel data segment */
    movw    %ax,   %ds
    movw    %ax,   %es

    movl    %esp,  %eax       /* pass register frame to C handler */
    pushl   %eax
    call    isr_handler
    addl    $4,    %esp

    popl    %gs
    popl    %fs
    popl    %es
    popl    %ds

    popa                      /* restore GPRs */
    addl    $8,    %esp       /* pop interrupt number and error code */
    iret

irq_common:
    pusha

    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs

    movw    $0x10, %ax
    movw    %ax,   %ds
    movw    %ax,   %es

    movl    %esp,  %eax
    pushl   %eax
    call    irq_handler
    addl    $4,    %esp

    popl    %gs
    popl    %fs
    popl    %es
    popl    %ds

    popa
    addl    $8,    %esp
    iret
