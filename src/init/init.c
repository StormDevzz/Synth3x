/* Synth3x OS — init (PID 1) — diagnostic version */

#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <linux/fb.h>
#include <linux/kd.h>

#define SYNTH3X_DE  "/usr/bin/synth3x"
#define SHELL       "/bin/sh"

/* VGA text buffer direct write (for diagnostics) */
static void vga_write(const char *s) {
    int fd = open("/dev/tty0", O_WRONLY);
    if (fd >= 0) { write(fd, s, strlen(s)); close(fd); }
}

/* ─── framebuffer helpers ─── */
#define RGB565(r,g,b) ((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3))

extern void splash_fast_fill(uint16_t *buf, int w, int h, uint16_t colour);
extern const uint8_t font8x8[];

static void put_char(uint16_t *fb, int stride, int x, int y,
                     char c, uint16_t fg, int scale) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *g = &font8x8[(c - 32) * 8];
    for (int r = 0; r < 8; r++)
        for (int cl = 0; cl < 8; cl++)
            if (g[r] & (0x80 >> cl))
                for (int dy = 0; dy < scale; dy++)
                    for (int dx = 0; dx < scale; dx++) {
                        int px = x + cl*scale + dx;
                        int py = y + r*scale + dy;
                        if (px >= 0 && px < stride)
                            fb[py*stride + px] = fg;
                    }
}

static void put_text(uint16_t *fb, int stride, int x, int y,
                     const char *s, uint16_t fg, int scale) {
    while (*s) { put_char(fb, stride, x, y, *s++, fg, scale); x += 8*scale; }
}

/* ─── Main ─── */
int main(int argc, char *argv[]) {
    vga_write("Synth3x init: starting\n");

    /* Mount filesystems */
    mkdir("/proc", 0755); mount("proc", "/proc", "proc", 0, NULL);
    mkdir("/sys", 0755);  mount("sysfs", "/sys", "sysfs", 0, NULL);
    mkdir("/dev", 0755);  mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    mkdir("/tmp", 0755);  mount("tmpfs", "/tmp", "tmpfs", 0, NULL);

    vga_write("Synth3x init: /dev mounted\n");

    /* Check /dev/fb0 */
    struct stat st;
    if (stat("/dev/fb0", &st) == 0) {
        vga_write("Synth3x init: /dev/fb0 exists\n");
    } else {
        vga_write("Synth3x init: /dev/fb0 NOT found\n");
    }

    /* Try to open framebuffer */
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) {
        vga_write("Synth3x init: cannot open /dev/fb0, falling back\n");
        goto fallback;
    }

    struct fb_var_screeninfo vi;
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vi) < 0) {
        vga_write("Synth3x init: FBIOGET failed\n");
        close(fb_fd); goto fallback;
    }

    int fb_w = vi.xres, fb_h = vi.yres;
    uint16_t *fb = mmap(NULL, fb_w*fb_h*2, PROT_READ|PROT_WRITE,
                        MAP_SHARED, fb_fd, 0);
    close(fb_fd);
    if (fb == MAP_FAILED) {
        vga_write("Synth3x init: mmap fb failed\n");
        goto fallback;
    }

    {
        char buf[128];
        snprintf(buf, sizeof(buf), "Synth3x init: fb %dx%d OK\n", fb_w, fb_h);
        vga_write(buf);
    }

    /* Switch to graphics mode */
    int tty = open("/dev/tty0", O_RDWR);
    if (tty >= 0) {
        ioctl(tty, KDSETMODE, KD_GRAPHICS);
        close(tty);
        vga_write("Synth3x init: graphics mode set\n");
    }

    /* Draw splash */
    uint16_t bg  = RGB565(12, 8, 22);
    uint16_t acc = RGB565(80, 140, 240);
    uint16_t txt = RGB565(200, 210, 240);
    uint16_t dim = RGB565(50, 40, 70);

    splash_fast_fill(fb, fb_w, fb_h, bg);
    for (int y = 0; y < fb_h; y++)
        for (int x = 0; x < fb_w; x += 2)
            fb[y*fb_w + x] = RGB565(
                20 + (fb_h-y)*30/fb_h,
                10 + (fb_h-y)*20/fb_h,
                30 + (fb_h-y)*40/fb_h);

    put_text(fb, fb_w, fb_w/2 - 4*8*3, fb_h/2 - 60, "S3", acc, 3);
    put_text(fb, fb_w, fb_w/2 - 4*8*3, fb_h/2 - 16, "Synth3x", txt, 2);
    for (int x = fb_w/2 - 100; x < fb_w/2 + 100; x++)
        fb[(fb_h/2 + 20)*fb_w + x] = acc;

    /* Try to launch Synth3x DE */
    {
        struct stat de_st;
        if (stat(SYNTH3X_DE, &de_st) == 0) {
            munmap(fb, fb_w*fb_h*2);
            execl(SYNTH3X_DE, "synth3x", NULL);
        }
        munmap(fb, fb_w*fb_h*2);
    }

fallback:
    vga_write("Synth3x init: starting shell\n");
    for (;;) {
        pid_t pid = fork();
        if (pid < 0) { sleep(3); continue; }
        if (pid == 0) { execl(SHELL, "sh", NULL); _exit(1); }
        int status;
        waitpid(pid, &status, 0);
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "init: shell exit %d, respawn\n", status);
            vga_write(buf);
        }
        sleep(2);
    }
}
