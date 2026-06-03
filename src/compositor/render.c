/* Synth3x Compositor — Render helpers (C wrappers for ASM)
 * Provides C-callable wrappers around assembly-optimized pixel routines.
 */

#include "compositor.h"

/* asm_fill_rect32 is defined in render.S:
 * void asm_fill_rect32(uint32_t *buf, int stride_pixels, int w, int h, uint32_t color);
 */

/* asm_fill_hline32 is defined in render.S:
 * void asm_fill_hline32(uint32_t *buf, int stride_pixels, int x, int y, int w, uint32_t color);
 */

/* asm_copy_rect32 is defined in render.S:
 * void asm_copy_rect32(uint32_t *dst, int dst_stride, uint32_t *src, int src_stride, int w, int h);
 */

/* asm_blend_rect32 is defined in render.S:
 * void asm_blend_rect32(uint32_t *dst, int stride, int w, int h, uint32_t color);
 */
