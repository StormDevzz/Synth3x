/* Synth3x DE — Fast framebuffer operations
 * x86-64, System V AMD64 ABI.
 *
 * Functions:
 *   fb_fill_rect  — fill a rectangle with RGB565 colour
 *   fb_fill_hline — draw a horizontal 1-pixel line (memset-like)
 *   fb_pixel      — set a single pixel with bounds check
 *   fb_blit_char  — blit an 8x8 character glyph from font bitmap
 */

.section .text

/* ─── fb_fill_rect ───────────────────────────────────────────────
 * void fb_fill_rect(uint16_t *buf, int stride, int fb_h,
 *                   int x, int y, int w, int h, uint16_t colour);
 *
 * Fill a rectangle.  Caller must ensure bounds.
 */
.global fb_fill_rect
fb_fill_rect:
    pushq   %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13

    movl    16(%rbp), %r10d      /* w */
    movl    24(%rbp), %r11d      /* h */
    movzwl  32(%rbp), %r12d      /* colour (zero-extend) */

    /* row_start = buf + y*stride + x */
    movl    %r8d,   %eax
    imull   %esi,   %eax
    addl    %ecx,   %eax
    leaq    (%rdi,%rax,2), %r13

    movl    %r11d, %ebx          /* row counter */

.Lfr_row:
    movq    %r13,  %rdi
    movl    %r10d, %ecx
    movzwl  %r12w, %eax
    rep     stosw

    /* r13 += stride (bytes) */
    movl    %esi,  %eax
    addl    %eax,  %eax
    addq    %rax,  %r13

    decl    %ebx
    jnz     .Lfr_row

    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret

/* ─── fb_fill_hline ──────────────────────────────────────────────
 * void fb_fill_hline(uint16_t *buf, int stride, int fb_h,
 *                    int x, int y, int w, uint16_t colour);
 */
.global fb_fill_hline
fb_fill_hline:
    movl    %r8d,  %eax           /* y */
    imull   %esi,  %eax           /* y * stride */
    addl    %ecx,  %eax           /* + x */
    leaq    (%rdi,%rax,2), %rdi

    movl    16(%rbp), %ecx        /* w */
    movzwl  24(%rbp), %eax        /* colour */
    rep     stosw
    ret

/* ─── fb_pixel ───────────────────────────────────────────────────
 * void fb_pixel(uint16_t *buf, int stride, int fb_h,
 *               int x, int y, uint16_t colour);
 */
.global fb_pixel
fb_pixel:
    cmpl    %edx,  %r8d
    jae     .Lpx_skip
    testl   %r8d,  %r8d
    js      .Lpx_skip

    movl    %ecx,  %eax
    testl   %eax,  %eax
    js      .Lpx_skip
    cmpl    %esi,  %eax
    jae     .Lpx_skip

    imull   %esi,  %r8d
    addl    %r8d,  %eax
    movzwl  16(%rbp), %r9d
    movw    %r9w,  (%rdi,%rax,2)

.Lpx_skip:
    ret

/* ─── fb_blit_char ───────────────────────────────────────────────
 * void fb_blit_char(uint16_t *buf, int stride, int fb_h,
 *                   int x, int y, const uint8_t *glyph,
 *                   uint16_t fg, uint16_t bg);
 *
 * Arg mapping (x86-64):
 *   %rdi = buf       %rsi = stride    %rdx = fb_h
 *   %rcx = x         %r8  = y         %r9  = glyph
 *   16(%rbp) = fg    24(%rbp) = bg
 */
.global fb_blit_char
fb_blit_char:
    pushq   %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14

    movzwl  16(%rbp), %r10d       /* fg */
    movzwl  24(%rbp), %r11d       /* bg */

    xorl    %r14d, %r14d          /* row = 0 */

.Lbc_row:
    movl    %r8d,   %r12d
    addl    %r14d,  %r12d         /* y + row */
    cmpl    %edx,   %r12d
    jae     .Lbc_next

    movl    %r12d, %eax
    imull   %esi,  %eax
    addl    %ecx,  %eax
    leaq    (%rdi,%rax,2), %r13

    movzbl  (%r9,%r14), %ebx      /* glyph byte */

    movl    $8,     %r12d         /* bit counter */

.Lbc_col:
    shll    $1,     %ebx
    jc      .Lbc_fg
    movw    %r11w,  (%r13)
    jmp     .Lbc_next_px
.Lbc_fg:
    movw    %r10w,  (%r13)
.Lbc_next_px:
    addq    $2,     %r13
    decl    %r12d
    jnz     .Lbc_col

.Lbc_next:
    incl    %r14d
    cmpl    $8,     %r14d
    jb      .Lbc_row

    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    ret
