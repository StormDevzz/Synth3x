/* Synth3x OS — Framebuffer abstraction library
 * For use with synth3x DE and other graphics programs
 */

#ifndef SYNTH3X_FB_H
#define SYNTH3X_FB_H

#include <stdint.h>

typedef struct {
    int fd;
    int width, height;
    int bytes_per_pixel;
    long screensize;
    uint8_t *buffer;
    uint8_t *back_buffer;
} Synth3xFB;

/* Open and map framebuffer */
Synth3xFB *fb_open(const char *device);

/* Double buffering */
void fb_swap(Synth3xFB *fb);

/* Drawing primitives */
void fb_pixel(Synth3xFB *fb, int x, int y, uint32_t color);
void fb_fill(Synth3xFB *fb, int x, int y, int w, int h, uint32_t color);
void fb_rect(Synth3xFB *fb, int x, int y, int w, int h, uint32_t color);
void fb_char(Synth3xFB *fb, int x, int y, char c, uint32_t color);
void fb_text(Synth3xFB *fb, int x, int y, const char *s, uint32_t color);

/* Color conversion */
uint32_t fb_rgb(uint8_t r, uint8_t g, uint8_t b);

/* Close framebuffer */
void fb_close(Synth3xFB *fb);

#endif /* SYNTH3X_FB_H */
