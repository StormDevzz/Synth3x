/* Synth3x OS — init (PID 1)
 * Mounts /dev, /proc, /sys; shows splash; forks Synth3x DE.
 * No system() calls — everything via direct syscall wrappers.
 */

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
#include <linux/input.h>
#include <dirent.h>

#define SYNTH3X_DE   "/usr/bin/synth3x"
#define SHELL        "/bin/sh"

extern void splash_fast_fill(uint16_t *buf, int w, int h, uint16_t colour);
extern const uint8_t font8x8[];

/* ─── Splash ─── */
#define RGB565(r,g,b) ((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3))

static void draw_char(uint16_t *fb, int stride, int x, int y,
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

static void draw_text(uint16_t *fb, int stride, int x, int y,
                      const char *s, uint16_t fg, int scale) {
    while (*s) {
        draw_char(fb, stride, x, y, *s++, fg, scale);
        x += 8 * scale;
    }
}

static void show_splash(uint16_t *fb, int w, int h) {
    uint16_t bg  = RGB565(12, 8, 22);
    uint16_t acc = RGB565(80, 140, 240);
    uint16_t txt = RGB565(200, 210, 240);
    uint16_t dim = RGB565(50, 40, 70);

    splash_fast_fill(fb, w, h, bg);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x += 2)
            fb[y * w + x] = RGB565(
                20 + (h - y) * 30 / h,
                10 + (h - y) * 20 / h,
                30 + (h - y) * 40 / h);

    draw_text(fb, w, w / 2 - 4 * 8 * 3, h / 2 - 60, "S3", acc, 3);
    draw_text(fb, w, w / 2 - 4 * 8 * 3, h / 2 - 16, "Synth3x", txt, 2);

    for (int x = w / 2 - 100; x < w / 2 + 100; x++)
        fb[(h / 2 + 20) * w + x] = acc;

    for (int i = 0; i < 3; i++)
        for (int dy = 0; dy < 6; dy++)
            for (int dx = 0; dx < 6; dx++)
                fb[(h - 32) * w + (w / 2 - 10 + i * 10 + dx)] = dim;
}

/* ─── System setup (no system() calls) ─── */
static void setup_system(void) {
    mkdir("/proc", 0755);
    mount("proc", "/proc", "proc", 0, NULL);
    mkdir("/sys", 0755);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mkdir("/dev", 0755);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    mkdir("/tmp", 0755);
    mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
    mkdir("/dev/pts", 0755);
    mount("devpts", "/dev/pts", "devpts", 0, NULL);

    int fd = open("/proc/sys/kernel/hostname", O_WRONLY);
    if (fd >= 0) { write(fd, "synth3x\n", 8); close(fd); }

    /* Symlink /bin/sh → busybox if needed */
    struct stat st;
    if (stat("/bin/sh", &st)) {
        symlink("/bin/busybox", "/bin/sh");
        symlink("/bin/busybox", "/bin/mount");
        symlink("/bin/busybox", "/bin/mkdir");
    }
}

/* ─── Framebuffer ─── */
static int fb_w, fb_h;
static uint16_t *fb_map;

static int setup_fb(void) {
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) return -1;
    struct fb_var_screeninfo vi;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vi)) { close(fd); return -1; }
    fb_w = vi.xres;
    fb_h = vi.yres;
    fb_map = mmap(NULL, fb_w * fb_h * 2, PROT_READ | PROT_WRITE,
                  MAP_SHARED, fd, 0);
    close(fd);
    if (fb_map == MAP_FAILED) return -1;
    return 0;
}

/* ─── Main ─── */
int main(int argc, char *argv[]) {
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGPIPE, SIG_IGN);

    setup_system();

    /* Try to get console in graphics mode */
    int tty = open("/dev/tty0", O_RDWR);
    if (tty >= 0) {
        ioctl(tty, KDSETMODE, KD_GRAPHICS);
        close(tty);
    }

    int fb_ok = (setup_fb() == 0);
    if (fb_ok) {
        show_splash(fb_map, fb_w, fb_h);
        usleep(600000);
        munmap(fb_map, fb_w * fb_h * 2);
    }

    printf("\nSynth3x OS — starting desktop environment...\n");

    for (;;) {
        pid_t pid = fork();
        if (pid < 0) { sleep(3); continue; }

        if (pid == 0) {
            struct stat de_st;
            if (stat(SYNTH3X_DE, &de_st) == 0) {
                execl(SYNTH3X_DE, "synth3x", NULL);
            }
            execl(SHELL, "sh", NULL);
            _exit(1);
        }

        int status;
        waitpid(pid, &status, 0);
        printf("init: child exited (%d), restarting...\n", status);
        sleep(2);
    }
}
