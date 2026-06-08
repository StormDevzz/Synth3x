/* Synth3x Compositor — Assembly-optimized pixel operations
 * x86-64, System V AMD64 ABI.
 *
 * All functions work on 32-bit RGBA pixel buffers.
 */

.section .text

/* ─── asm_fill_rect32 ─────────────────────────────────────────────
 * void asm_fill_rect32(uint32_t *buf, int stride, int w, int h, uint32_t color);
 * Fill a rectangle with a solid 32-bit color.
 */
.global asm_fill_rect32
asm_fill_rect32:
    pushq   %rbx

    movl    %edx,   %eax          /* w */
    shll    $2,     %eax          /* w * 4 (bytes per row) */
    movl    %esi,   %ebx          /* stride */
    shll    $2,     %ebx          /* stride * 4 */
    subl    %eax,   %ebx          /* ebx = (stride - w) * 4 (row offset in bytes) */
    movslq  %ebx,   %rbx          /* sign extend to 64-bit */

.Lfill_row:
    movl    %edx,   %r9d          /* loop counter for row (w) */
.Lfill_pixel:
    movl    %r8d,   (%rdi)
    addq    $4,     %rdi
    decl    %r9d
    jnz     .Lfill_pixel

    addq    %rbx,   %rdi          /* skip to next row (safe 64-bit pointer arithmetic) */
    decl    %ecx                  /* decrement h */
    jnz     .Lfill_row

    popq    %rbx
    ret

/* ─── asm_fill_hline32 ────────────────────────────────────────────
 * void asm_fill_hline32(uint32_t *buf, int stride, int x, int y, int w, uint32_t color);
 */
.global asm_fill_hline32
asm_fill_hline32:
    pushq   %rbx
    movslq  %ecx,   %rax          /* y (64-bit) */
    movslq  %esi,   %rbx          /* stride (64-bit) */
    imulq   %rbx,   %rax          /* y * stride */
    movslq  %edx,   %rdx          /* x (64-bit) */
    addq    %rdx,   %rax          /* + x */
    leaq    (%rdi,%rax,4), %rdi   /* rdi = buf + (y * stride + x) * 4 */

    movl    %r8d,   %ecx          /* w */
    movl    %r9d,   %eax          /* color */
    rep     stosl
    popq    %rbx
    ret

/* ─── asm_copy_rect32 ─────────────────────────────────────────────
 * void asm_copy_rect32(uint32_t *dst, int dst_stride,
 *                      uint32_t *src, int src_stride, int w, int h);
 * Copy a rectangle from src to dst using REP MOVSD.
 */
.global asm_copy_rect32
asm_copy_rect32:
    pushq   %rbx
    pushq   %r12
    pushq   %r13

    movl    %esi,   %r12d         /* r12d = dst_stride */
    movl    %ecx,   %r13d         /* r13d = src_stride */

    /* Calculate row offsets in bytes */
    /* dst offset = (dst_stride - w) * 4 */
    /* src offset = (src_stride - w) * 4 */
    subl    %r8d,   %r12d         /* dst_stride - w */
    shll    $2,     %r12d         /* (dst_stride - w) * 4 */
    movslq  %r12d,  %r12          /* dst offset */

    subl    %r8d,   %r13d         /* src_stride - w */
    shll    $2,     %r13d         /* (src_stride - w) * 4 */
    movslq  %r13d,  %r13          /* src offset */

    movq    %rdx,   %rsi          /* rsi = src pointer */

.Lcpy_row:
    movl    %r8d,   %ecx          /* ecx = w */
    rep     movsl

    addq    %r12,   %rdi          /* dst = dst + dst_offset */
    addq    %r13,   %rsi          /* src = src + src_offset */

    decl    %r9d                  /* decrement h */
    jnz     .Lcpy_row

    popq    %r13
    popq    %r12
    popq    %rbx
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
    pushq   %r15

    movl    %edx,   %r10d         /* r10d = w */
    movl    %ecx,   %r11d         /* r11d = h */

    /* Extract constant source components from color (%r8d) */
    movl    %r8d,   %eax
    movzbl  %al,    %r14d         /* r14d = src_b */
    shrl    $8,     %eax
    movzbl  %al,    %r13d         /* r13d = src_g */
    shrl    $8,     %eax
    movzbl  %al,    %r12d         /* r12d = src_r */
    shrl    $8,     %eax
    movzbl  %al,    %r9d          /* r9d  = src_a (alpha) */

    /* Calculate row offset in bytes: r15 = (stride - w) * 4 */
    movl    %esi,   %r15d
    subl    %r10d,  %r15d
    shll    $2,     %r15d
    movslq  %r15d,  %r15          /* r15 = 64-bit offset */

.Lblend_row:
    movl    %r10d,  %r8d          /* r8d = loop counter (w) */
.Lblend_px:
    movl    (%rdi), %eax          /* eax = dst_color (0xFF_RR_GG_BB) */

    /* Blend Blue */
    movzbl  %r9b,   %ecx          /* ecx = alpha */
    movl    %r14d,  %ebx          /* ebx = src_b */
    imull   %ecx,   %ebx          /* ebx = src_b * alpha */

    movzbl  %al,    %edx          /* edx = dst_b */
    imull   $255,   %edx          /* edx = dst_b * 255 */

    movzbl  %al,    %ebp          /* temp ebp = dst_b */
    imull   %ecx,   %ebp          /* temp = dst_b * alpha */
    subl    %ebp,   %edx          /* edx = dst_b * (255 - alpha) */
    addl    %ebx,   %edx
    shrl    $8,     %edx          /* edx = new_b */

    /* Blend Green */
    movl    %r13d,  %ebx          /* ebx = src_g */
    imull   %ecx,   %ebx          /* ebx = src_g * alpha */

    movzbl  %ah,    %ebp          /* ebp = dst_g */
    movl    %ebp,   %eax          /* eax = dst_g */
    imull   $255,   %ebp          /* ebp = dst_g * 255 */
    imull   %ecx,   %eax          /* eax = dst_g * alpha */
    subl    %eax,   %ebp          /* ebp = dst_g * (255 - alpha) */
    addl    %ebx,   %ebp          /* ebp = new_g * 256 */
    shrl    $8,     %ebp
    shll    $8,     %ebp
    orl     %ebp,   %edx          /* merge green into edx */

    /* Blend Red */
    movl    %r12d,  %ebx          /* ebx = src_r */
    imull   %ecx,   %ebx          /* ebx = src_r * alpha */

    movl    (%rdi), %eax          /* reload dst_color to extract Red */
    shrl    $16,    %eax
    movzbl  %al,    %ebp          /* ebp = dst_r */
    movl    %ebp,   %eax          /* eax = dst_r */
    imull   $255,   %ebp          /* ebp = dst_r * 255 */
    imull   %ecx,   %eax          /* eax = dst_r * alpha */
    subl    %eax,   %ebp          /* ebp = dst_r * (255 - alpha) */
    addl    %ebx,   %ebp
    shrl    $8,     %ebp
    shll    $16,    %ebp
    orl     %ebp,   %edx          /* merge red into edx */

    /* Force alpha to 0xFF */
    orl     $0xFF000000, %edx

    /* Write back pixel */
    movl    %edx,   (%rdi)
    addq    $4,     %rdi

    decl    %r8d
    jnz     .Lblend_px

    addq    %r15,   %rdi          /* stride to next row */
    decl    %r11d
    jnz     .Lblend_row

    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret
