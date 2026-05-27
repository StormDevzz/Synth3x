/* Synth3x OS — Multiboot2 header
 * Assembly — must come first in the binary
 */

.section .multiboot, "a"
.code32

.align 8

/* Multiboot2 header */
.set MAGIC,         0xE85250D6
.set ARCHITECTURE,  0  /* x86 (32-bit protected mode) */
.set HEADER_LENGTH, header_end - header_start
.set CHECKSUM,      -(MAGIC + ARCHITECTURE + HEADER_LENGTH)

header_start:
    .long MAGIC
    .long ARCHITECTURE
    .long HEADER_LENGTH
    .long CHECKSUM

    /* End tag */
    .word 0   /* type */
    .word 0   /* flags */
    .long 8   /* size */
header_end:

/* Entry point */
.section .text
.global _start
_start:
    /* Stack setup */
    mov $stack_top, %esp
    mov $stack_top, %ebp

    /* Push multiboot info pointer (ebx) and magic (eax) */
    push %ebx
    push %eax

    /* Call kernel_main */
    call kernel_main

    /* Should never reach here */
    cli
hang:
    hlt
    jmp hang

.section .bss
.align 16
stack_bottom:
    .skip 16384  /* 16KB stack */
stack_top:
