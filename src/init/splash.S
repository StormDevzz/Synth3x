/* Synth3x OS — Fast framebuffer fill for boot splash
 * x86-64, System V AMD64 ABI.
 *
 * splash_fast_fill(buf, w, h, colour)  — rep stosw fill
 * splash_pixel(buf, stride, x, y, colour) — single pixel
 */

.section .text

.global splash_fast_fill
splash_fast_fill:
    movzwl  %cx,   %r8d        /* colour was in %ecx */
    movl    %esi,  %eax
    mull    %edx               /* w * h → eax */
    movl    %eax,  %ecx        /* pixel count for rep stosw */
    movzwl  %r8w,  %eax        /* colour */
    rep     stosw
    ret

.global splash_pixel
splash_pixel:
    movl    %ecx,  %eax        /* y (4th arg) */
    imull   %esi,  %eax        /* y * stride */
    addl    %edx,  %eax        /* + x (3rd arg) */
    movzwl  %r8w,  %r9d        /* colour (5th arg) */
    movw    %r9w,  (%rdi,%rax,2)
    ret
