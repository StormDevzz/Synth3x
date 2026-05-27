/* Synth3x OS — Optimised memory operations
 * Calling convention: cdecl (i386)
 * Implemented with `rep movs` / `rep stos` for speed.
 */

.section .text

/* void *memset(void *s, int c, size_t n) */
.global memset
memset:
    pushl   %ebp
    movl    %esp,   %ebp
    pushl   %edi

    movl    8(%ebp),  %edi     /* s */
    movl    12(%ebp), %eax     /* c  (byte value) */
    movl    16(%ebp), %ecx     /* n */
    movb    %al,     %ah       /* replicate byte across eax */
    movl    %eax,    %edx
    shll    $16,     %edx
    orl     %edx,    %eax

    testl   %ecx, %ecx
    jz      1f

    /* align to 4 bytes */
    movl    %edi,    %edx
    andl    $3,      %edx
    jz      2f
    negl    %edx
    addl    $4,      %edx
    cmpl    %edx,    %ecx
    jb      3f
    subl    %edx,    %ecx
    rep     stosb

2:  /* do 4-byte stores */
    movl    %ecx,    %edx
    shrl    $2,      %ecx
    rep     stosl
    movl    %edx,    %ecx
    andl    $3,      %ecx

3:  /* remaining bytes */
    rep     stosb

1:
    movl    8(%ebp), %eax      /* return s */
    popl    %edi
    popl    %ebp
    ret

/* void *memcpy(void *dest, const void *src, size_t n) */
.global memcpy
memcpy:
    pushl   %ebp
    movl    %esp,   %ebp
    pushl   %esi
    pushl   %edi

    movl    8(%ebp),  %edi     /* dest */
    movl    12(%ebp), %esi     /* src */
    movl    16(%ebp), %ecx     /* n */

    testl   %ecx, %ecx
    jz      1f

    /* check alignment */
    movl    %edi,    %eax
    orl     %esi,    %eax
    andl    $3,      %eax
    jnz     2f

    /* 4-byte copy */
    movl    %ecx,    %edx
    shrl    $2,      %ecx
    rep     movsl
    movl    %edx,    %ecx
    andl    $3,      %ecx

2:  /* byte copy */
    rep     movsb

1:
    movl    8(%ebp), %eax      /* return dest */
    popl    %edi
    popl    %esi
    popl    %ebp
    ret

/* void *memmove(void *dest, const void *src, size_t n) */
.global memmove
memmove:
    pushl   %ebp
    movl    %esp,   %ebp
    pushl   %esi
    pushl   %edi

    movl    8(%ebp),  %edi     /* dest */
    movl    12(%ebp), %esi     /* src */
    movl    16(%ebp), %ecx     /* n */

    testl   %ecx, %ecx
    jz      1f

    cmpl    %edi, %esi
    jae     forward
    /* overlap: copy backwards */
    addl    %ecx, %edi
    addl    %ecx, %esi
    decl    %edi
    decl    %esi
    std
    rep     movsb
    cld
    jmp     1f

forward:
    /* check alignment */
    movl    %edi, %eax
    orl     %esi, %eax
    andl    $3,   %eax
    jnz     2f

    movl    %ecx, %edx
    shrl    $2,   %ecx
    rep     movsl
    movl    %edx, %ecx
    andl    $3,   %ecx

2:  rep     movsb

1:
    movl    8(%ebp), %eax      /* return dest */
    popl    %edi
    popl    %esi
    popl    %ebp
    ret
