.section .multiboot, "a"
.code32

.align 4
.long 0x1BADB002
.long 0x00000003
.long -(0x1BADB002 + 0x00000003)

.section .note.Xen, "a", @note
.align 4
.long 4
.long 4
.long 3
.asciz "Xen"
.long _start

.section .text
.global _start
_start:
mov $stack_top, %esp
mov $stack_top, %ebp

cmpl $0x2BADB002, %eax
je 1f
xorl %ebx, %ebx
xorl %eax, %eax
1:
push %ebx
push %eax
call kernel_main

cli
hang:
hlt
jmp hang

.section .bss
.align 16
stack_bottom:
.skip 32768
stack_top:
