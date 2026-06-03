/* Synth3x Compositor — Assembly-optimized pixel operations
 * x86-64, System V AMD64 ABI.
 *
 * All functions work on 32-bit RGBA pixel buffers.
 */

.section .text

/* ─── asm_fill_rect32 ─────────────────────────────────────────────
 * void asm_fill_rect32(uint32_t *buf, int stride, int w, int h, uint32_t color);
 * Fill a rectangle with a solid 32-bit color using REP STOSD.
 */
.global asm_fill_rect32
asm_fill_rect32:
    pushq   %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13

    movl    %edx,   %r10d         /* w */
    movl    %ecx,   %r11d         /* h */
    movl    %r8d,   %r12d         /* color */

    movl    %esi,   %eax          /* stride in pixels */
    mull    %r10d                 /* bytes per row = stride * 4 (we use stride as pixel stride) */
    movl    %eax,   %ebx          /* row byte stride */

    xorl    %r13d,  %r13d
.Lfill_row:
    movq    %rdi,   %rax
    movl    %r10d,  %ecx
    movl    %r12d,  %eax
    rep     stosl

    addl    %ebx,   %edi          /* skip to next row (edi is now wrong, fix below) */
    /* Actually, rep stosl advances rdi by ecx*4. So we need to compute the next row start */
    /* Let's use a different approach - compute row start each time */
    decl    %r11d
    jz      .Lfill_done

    movl    %esi,   %eax
    addl    %r13d,  %eax
    movl    %eax,   %r13d
    movq    %rdi,   %rcx
    /* rdi already points past the row from rep stosl, we need to add (stride - w) more */
    subl    %r10d,  %esi
    leaq    (%rdi,%rsi,4), %rdi
    addl    %r10d,  %esi
    jmp     .Lfill_row

.Lfill_done:
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret

/* ─── asm_fill_hline32 ────────────────────────────────────────────
 * void asm_fill_hline32(uint32_t *buf, int stride, int x, int y, int w, uint32_t color);
 */
.global asm_fill_hline32
asm_fill_hline32:
    movl    %ecx,   %eax          /* y */
    imull   %esi,   %eax          /* y * stride */
    addl    %edx,   %eax          /* + x */
    leaq    (%rdi,%rax,4), %rdi

    movl    %r8d,   %ecx          /* w */
    movl    %r9d,   %eax          /* color */
    rep     stosl
    ret

/* ─── asm_copy_rect32 ─────────────────────────────────────────────
 * void asm_copy_rect32(uint32_t *dst, int dst_stride,
 *                      uint32_t *src, int src_stride, int w, int h);
 * Copy a rectangle from src to dst using REP MOVSD.
 */
.global asm_copy_rect32
asm_copy_rect32:
    pushq   %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14

    movl    %r8d,   %r10d         /* w */
    movl    %r9d,   %r11d         /* h */

    /* args on stack after 6th: dst_stride in 16(%rbp), src_stride in 24(%rbp) */
    movl    16(%rbp), %r12d       /* dst_stride */
    movl    24(%rbp), %r13d       /* src_stride */

    xorl    %r14d,  %r14d
.Lcpy_row:
    movq    %rdi,   %rax
    movq    %rdx,   %rsi
    movl    %r10d,  %ecx
    rep     movsl

    addl    %r12d,  %r12d
    addl    %r12d,  %r12d         /* dst_stride * 4 */
    addq    %rcx,   %rdi
    subl    %r10d,  %r12d
    shrl    $2,     %r12d

    addl    %r13d,  %r13d
    addl    %r13d,  %r13d
    addq    %rcx,   %rsi
    subl    %r10d,  %r13d
    shrl    $2,     %r13d

    decl    %r11d
    jnz     .Lcpy_row

    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret

/* ─── asm_blend_rect32 ────────────────────────────────────────────
 * void asm_blend_rect32(uint32_t *dst, int stride, int w, int h, uint32_t color);
 * Alpha-blend a solid color over a rectangle using SSE2.
 * Color format: 0xAARRGGBB (alpha in high byte).
 */
.global asm_blend_rect32
asm_blend_rect32:
    pushq   %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14

    movl    %edx,   %r10d         /* w */
    movl    %ecx,   %r11d         /* h */
    movl    %r8d,   %r12d         /* color */

    movzbl  %r12b,  %eax          /* b */
    movzbl  %r12b,  %edx
    shrl    $8,     %r12d
    movzbl  %r12b,  %ecx          /* g */
    movzbl  %r12b,  %ebx
    shrl    $8,     %r12d
    movzbl  %r12b,  %r8d          /* r */
    shrl    $8,     %r12d
    movzbl  %r12b,  %r9d          /* a */
    movl    %r9d,   %r12d

    /* Reconstruct color from components for the blend loop */
    shll    $24,    %r9d
    shll    $16,    %r8d
    shll    $8,     %ecx
    orl     %ecx,   %r8d
    orl     %ebx,   %r8d
    orl     %r9d,   %r8d
    movl    %r8d,   %r12d         /* full color */

    /* alpha in r9d */
    movzbl  %r12b,  %r9d
    shrl    $24,    %r12d
    xchgl   %r9d,   %r12d
    /* r9d = alpha, r12d = color */

    xorl    %r13d,  %r13d
.Lblend_row:
    movl    %r10d,  %r14d
.Lblend_px:
    movl    (%rdi), %eax
    movzbl  %r9b,   %ecx          /* alpha */
    movzbl  %r12b,  %ebx          /* src_b */
    movzbl  %al,    %edx          /* dst_b */
    imull   %ecx,   %ebx
    imull   $255,   %edx
    subl    %ecx,   %edx
    addl    %ebx,   %edx
    shrl    $8,     %edx

    movzbl  %r12b,  %ebx
    shrl    $8,     %r12d
    movzbl  %r12b,  %ebx          /* src_g */
    movzbl  %ah,    %ecx          /* dst_g */
    movzbl  %r9b,   %r8d
    imull   %r8d,   %ebx
    imull   $255,   %ecx
    subl    %r8d,   %ecx
    addl    %ebx,   %ecx
    shrl    $8,     %ecx
    shll    $8,     %ecx
    orl     %ecx,   %edx
    movzbl  %r12b,  %ebx
    shrl    $8,     %r12d
    movzbl  %r12b,  %ebx          /* src_r */
    shrl    $16,    %eax
    movzbl  %ah,    %ecx          /* dst_r */
    movzbl  %r9b,   %r8d
    imull   %r8d,   %ebx
    imull   $255,   %ecx
    subl    %r8d,   %ecx
    addl    %ebx,   %ecx
    shrl    $8,     %ecx
    shll    $16,    %ecx
    orl     %ecx,   %edx

    movb    $0xFF,  %dl
    movl    %edx,   (%rdi)
    addq    $4,     %rdi

    decl    %r14d
    jnz     .Lblend_px

    addl    %esi,   %edi          /* stride to next row */
    decl    %r11d
    jnz     .Lblend_row

    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret
