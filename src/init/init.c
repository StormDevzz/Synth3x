/* Synth3x OS — init process
 * First userspace process.  Shows a bootsplash, then starts the DE.
 */

#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdint.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <time.h>

#define SYNTH3X_DE   "/usr/bin/synth3x"
#define XFCE_SESSION "/usr/bin/startxfce4"
#define SHELL        "/bin/sh"

/* ASM routines from splash.S */
extern void splash_fast_fill(uint16_t *buf, int w, int h, uint16_t colour);
extern void splash_pixel(uint16_t *buf, int stride, int x, int y, uint16_t colour);

/* ASM font data from font.S */
extern const uint8_t font8x8[];

/* ─── Helpers ─── */
static int file_exists(const char *p) {
    struct stat st;
    return stat(p, &st) == 0;
}

/* ─── Bootsplash ─── */
#define RGB565(r,g,b) ((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3))

static void splash_char(uint16_t *fb, int stride, int x, int y,
                        char c, uint16_t fg, int scale) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *g = &font8x8[(c - 32) * 8];
    for (int r = 0; r < 8; r++)
        for (int cl = 0; cl < 8; cl++)
            if (g[r] & (0x80 >> cl))
                for (int dy = 0; dy < scale; dy++)
                    for (int dx = 0; dx < scale; dx++) {
                        int px = x + cl * scale + dx;
                        int py = y + r * scale + dy;
                        if (px >= 0 && px < stride)
                            fb[py * stride + px] = fg;
                    }
}

static void splash_text(uint16_t *fb, int stride, int x, int y,
                        const char *s, uint16_t fg, int scale) {
    while (*s) {
        splash_char(fb, stride, x, y, *s++, fg, scale);
        x += 8 * scale;
    }
}

static void show_splash(uint16_t *fb, int w, int h) {
    uint16_t bg = RGB565(15, 10, 25);
    uint16_t accent = RGB565(80, 140, 240);
    uint16_t text = RGB565(200, 210, 240);
    uint16_t dim = RGB565(60, 50, 80);

    splash_fast_fill(fb, w, h, bg);

    /* gradient overlay */
    for (int y = 0; y < h; y++) {
        uint16_t t = (y < h / 2)
            ? RGB565(80 - y * 30 / h, 140 - y * 70 / h, 240 - y * 120 / h)
            : RGB565(50, 30, 60);
        for (int x = 0; x < w; x += 4)
            fb[y * w + x] = t;
    }

    /* logo — large "S3" */
    splash_text(fb, w, w / 2 - 4 * 8 * 3, h / 2 - 60, "S3", accent, 3);
    splash_text(fb, w, w / 2 - 4 * 8 * 3, h / 2 - 20, "Synth3x", text, 2);

    /* accent line */
    for (int x = w / 2 - 120; x < w / 2 + 120; x++)
        fb[(h / 2 + 20) * w + x] = accent;

    /* loading dots */
    for (int i = 0; i < 3; i++)
        for (int dy = 0; dy < 8; dy++)
            for (int dx = 0; dx < 8; dx++)
                fb[(h - 40) * w + (w / 2 - 12 + i * 12 + dx)] = dim;
}

/* ─── Framebuffer init ─── */
static int fb_w, fb_h;
static uint16_t *fb_map;

static int setup_fb(void) {
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) return -1;
    struct fb_var_screeninfo vi;
    ioctl(fd, FBIOGET_VSCREENINFO, &vi);
    fb_w = vi.xres;
    fb_h = vi.yres;
    fb_map = mmap(NULL, fb_w * fb_h * 2, PROT_READ | PROT_WRITE,
                  MAP_SHARED, fd, 0);
    close(fd);
    if (fb_map == MAP_FAILED) return -1;
    return 0;
}

/* ─── System setup ─── */
static void setup_system(void) {
    system("mount -t proc proc /proc 2>/dev/null");
    system("mount -t sysfs sysfs /sys 2>/dev/null");
    system("mount -t devtmpfs devtmpfs /dev 2>/dev/null");
    system("mount -t tmpfs tmpfs /tmp 2>/dev/null");
    system("mkdir -p /dev/pts /var/log 2>/dev/null");
    system("mount -t devpts devpts /dev/pts 2>/dev/null");

    int fd = open("/proc/sys/kernel/hostname", O_WRONLY);
    if (fd >= 0) { write(fd, "synth3x\n", 8); close(fd); }
    system("ip link set lo up 2>/dev/null");
}

/* ─── Main ─── */
int main(int argc, char *argv[]) {
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    setup_system();
    setup_fb();

    if (fb_map) show_splash(fb_map, fb_w, fb_h);
    usleep(800000);
    if (fb_map) munmap(fb_map, fb_w * fb_h * 2);

    /* Fork so that the DE/shell is NOT PID 1 — avoids kernel panic */
    for (;;) {
        pid_t pid = fork();
        if (pid < 0) { sleep(5); continue; }

        if (pid == 0) {
            /* Child — run DE or shell */
            if (file_exists(SYNTH3X_DE)) {
                execl(SYNTH3X_DE, "synth3x", NULL);
                execl(SHELL, "sh", NULL);
            }
            execl(SHELL, "sh", NULL);
            _exit(1);
        }

        /* Parent (PID 1) — wait for child and respawn */
        int status;
        waitpid(pid, &status, 0);
        printf("init: DE exited (status %d), respawning...\n", status);
        sleep(2);
    }
}
