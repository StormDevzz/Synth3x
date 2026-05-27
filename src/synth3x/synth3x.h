#ifndef SYNTH3X_ASM_H
#define SYNTH3X_ASM_H

#include <stdint.h>

/* fb_asm.S — framebuffer drawing primitives */
void fb_fill_rect(uint16_t *buf, int stride, int fb_h,
                  int x, int y, int w, int h, uint16_t colour);
void fb_fill_hline(uint16_t *buf, int stride, int fb_h,
                   int x, int y, int w, uint16_t colour);
void fb_pixel(uint16_t *buf, int stride, int fb_h,
              int x, int y, uint16_t colour);
void fb_blit_char(uint16_t *buf, int stride, int fb_h,
                  int x, int y, const uint8_t *glyph,
                  uint16_t fg, uint16_t bg);

/* font.S — 8x8 bitmap font (ASCII 32–126, 95 glyphs × 8 bytes) */
extern const uint8_t font8x8[];

#endif
