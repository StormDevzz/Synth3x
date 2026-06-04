/* Synth3x Compositor — Main Entry Point
 * Wayland compositor with AmnesiaDE shell and DRM/KMS backend.
 * Replaces the old framebuffer-based synth3x.c
 */

#include "compositor.h"
#include <linux/fb.h>
#include <sys/ioctl.h>

int compositor_init(compositor_t *c) {
    memset(c, 0, sizeof(*c));
    
    /* Initialize DRM/KMS display */
    if (drm_init(c) < 0) {
        fprintf(stderr, "Compositor: DRM init failed, trying fbdev\n");
        int fb_fd = open("/dev/fb0", O_RDWR);
        if (fb_fd < 0) {
            fprintf(stderr, "Compositor: No display device found\n");
            return -1;
        }
        struct fb_var_screeninfo vi;
        if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vi) == 0 && vi.xres > 0 && vi.yres > 0) {
            c->fb_w = vi.xres;
            c->fb_h = vi.yres;
        } else {
            c->fb_w = 1024;
            c->fb_h = 768;
        }
        c->drm.fd = fb_fd;
        c->drm.stride = c->fb_w * 4;
        c->drm.size = c->fb_h * c->fb_w * 4;
        c->backbuf_size = c->drm.size;
        c->backbuf = malloc(c->drm.size);
        c->drm.map = mmap(NULL, c->drm.size, PROT_READ|PROT_WRITE,
                          MAP_SHARED, fb_fd, 0);
        if (c->drm.map == MAP_FAILED) {
            free(c->backbuf);
            close(fb_fd);
            return -1;
        }
        fprintf(stderr, "Compositor: fbdev fallback %dx%d\n",
                c->fb_w, c->fb_h);
    }
    
    input_init(c);
    
    if (wl_server_init(c) < 0)
        fprintf(stderr, "Compositor: Wayland init failed\n");
    
    shell_init(c);
    
    c->tty_fd = open("/dev/tty0", O_RDWR);
    
    fprintf(stderr, "Compositor: initialized (%dx%d)\n", c->fb_w, c->fb_h);
    return 0;
}

void compositor_run(compositor_t *c) {
    struct timespec last = {0, 0};
    
    while (c->running) {
        /* Poll input devices */
        input_poll(c);
        
        /* Poll Wayland clients */
        wl_server_poll(c);
        
        /* Handle mouse button clicks */
        if (c->mclick) {
            c->mclick = 0;
            shell_handle_click(c);
        }
        
        /* Handle drag end on mouse release */
        if (!c->mouse_pressed) {
            for (int i = 0; i < c->wc; i++)
                c->wins[i].drag = 0;
            if (c->selecting) {
                c->selecting = 0;
                int dx = abs(c->mx - c->sel_start_x);
                int dy = abs(c->my - c->sel_start_y);
                if (dx > 8 || dy > 8) {
                    extract_selected_text(c);
                    if (strlen(c->selected_text) > 0) {
                        c->show_copy_dialog = 1;
                    }
                }
            }
        }
        
        /* Frame rate limiting (~60fps) */
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed = (now.tv_sec - last.tv_sec) * 1000000000L +
                      (now.tv_nsec - last.tv_nsec);
        if (elapsed < 16000000) { /* 16ms = ~60fps */
            usleep((16000000 - elapsed) / 1000);
            continue;
        }
        last = now;
        
        /* Render frame */
        compositor_frame(c);
        
        /* Swap buffers */
        drm_swap(c);
    }
}

void compositor_frame(compositor_t *c) {
    /* Clear backbuffer */
    memset(c->backbuf, 0, c->fb_w * c->fb_h * 4);
    
    /* Draw shell/desktop */
    shell_draw(c);
}

void compositor_shutdown(compositor_t *c) {
    fprintf(stderr, "Compositor: shutting down...\n");
    
    wl_server_shutdown(c);
    input_shutdown(c);
    drm_shutdown(c);
    
    if (c->tty_fd >= 0) close(c->tty_fd);
    if (c->notif_fd >= 0) close(c->notif_fd);
    unlink("/tmp/synth3x-notif");
}

int main(int argc, char *argv[]) {
    fprintf(stderr, "Synth3x Compositor v%s — Wayland + DRM/KMS\n",
            SYNTH3X_VERSION);
    
    compositor_t comp;
    if (compositor_init(&comp) < 0) {
        fprintf(stderr, "Compositor: initialization failed\n");
        return 1;
    }
    
    compositor_run(&comp);
    compositor_shutdown(&comp);
    
    fprintf(stderr, "Compositor: done.\n");
    return 0;
}
