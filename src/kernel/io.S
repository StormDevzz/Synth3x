/* Synth3x OS — Port I/O functions
 * Calling convention: cdecl (i386)
 */

.section .text

/* uint8_t inb(uint16_t port) */
.global inb
inb:
    pushl   %ebp
    movl    %esp, %ebp
    xorl    %eax, %eax
    movl    8(%ebp), %edx
    inb     %dx, %al
    popl    %ebp
    ret

/* uint16_t inw(uint16_t port) */
.global inw
inw:
    pushl   %ebp
    movl    %esp, %ebp
    xorl    %eax, %eax
    movl    8(%ebp), %edx
    inw     %dx, %ax
    popl    %ebp
    ret

/* uint32_t inl(uint16_t port) */
.global inl
inl:
    pushl   %ebp
    movl    %esp, %ebp
    xorl    %eax, %eax
    movl    8(%ebp), %edx
    inl     %dx, %eax
    popl    %ebp
    ret

/* void outb(uint16_t port, uint8_t val) */
.global outb
outb:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp),  %edx
    movl    12(%ebp), %eax
    outb    %al, %dx
    popl    %ebp
    ret

/* void outw(uint16_t port, uint16_t val) */
.global outw
outw:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp),  %edx
    movl    12(%ebp), %eax
    outw    %ax, %dx
    popl    %ebp
    ret

/* void outl(uint16_t port, uint32_t val) */
.global outl
outl:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp),  %edx
    movl    12(%ebp), %eax
    outl    %eax, %dx
    popl    %ebp
    ret
