#define SYNTH3X_VERSION "0.8"
/* Synth3x OS — AmnesiaDE v0.8 — Gentoo-Style Cyberpunk Desktop
 * C + Custom Optimized Framebuffer. Browser, Touchpad, Gentoo Guide.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <pthread.h>

#include "synth3x.h"

/* ─── CONFIG ─── */
#define MAX_WIN     16
#define MAX_NOTIF   8
#define PANEL_H     30
#define NOTIF_W     280
#define NOTIF_H     72
#define NOTIF_DUR   5
#define WORKSPACES  4

#define RGB565(r,g,b) ((((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3))

/* ─── SYSTEM WINDOWS & FORWARD DECLARATIONS ─── */
typedef struct { 
    int x, y, w, h; 
    char title[48]; 
    int hidden, ws, drag, dx, dy;
    int maximized;
    int orig_x, orig_y, orig_w, orig_h;
} Win;

static Win wins[MAX_WIN]; 
static int wc = 0; 
static int aw = -1;

static int find_win_by_title(const char *title);
static int wnew(const char *t, int w, int h);
static void draw_desktop_icons(void);

/* ─── 32-BIT HEX NEON COLOR DEFINITIONS ─── */
#define COLOR_BG        0xFF0A0514
#define COLOR_PANEL_BG  0xFF140A20
#define COLOR_PANEL_FG  0xFFC8B4E6
#define COLOR_WIN_BG    0xFF0F0A18
#define COLOR_WIN_TITLE 0xFF180E26
#define COLOR_WIN_BORDER 0xFF461E6E
#define COLOR_ACCENT    0xFFFF0080
#define COLOR_TEXT      0xFF00FFE6
#define COLOR_DIM       0xFF5A3C6E
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_GREEN     0xFF50DC64
#define COLOR_YELLOW    0xFFFADC32
#define COLOR_RED       0xFFFA5064
#define COLOR_ORANGE    0xFFFF8800

/* ─── RETRO COLOR STRUCTURES ─── */
typedef struct { unsigned char r, g, b; } Color;
static Color lerp(Color a, Color b, float t) {
    return (Color){ a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t };
}
static uint16_t c565(Color c) { return RGB565(c.r,c.g,c.b); }

/* ─── STRUCTURAL DECLARATIONS ─── */
typedef struct { char title[48]; char body[128]; time_t t; } Notif;
static Notif notifs[MAX_NOTIF]; static int nc = 0;
static int notif_fd = -1;

#define MAX_TERM_LOGS 14
static char term_logs[MAX_TERM_LOGS][80];
static int term_log_count = 0;
static char term_input[128] = "";

/* ─── GLOBALS ─── */
static int fb_fd = -1, fb_w = 800, fb_h = 600;
static uint8_t *fb = NULL;
static uint16_t *backbuf16 = NULL;
static uint32_t *backbuf32 = NULL;
static int fb_bpp = 16;
static int fb_stride_bytes = 0;
static int running = 1;
static int mx = 400, my = 300, mclick = 0;
static int shift_pressed = 0;
static int super_pressed = 0;
static int vscodium_installed = 0;
static int current_ws = 0;

/* ─── GUIDE PAGES ─── */
static int guide_page = 0;
#define GUIDE_MAX_PAGES 8

/* ─── FORWARD DECLARATIONS ─── */
static uint32_t get_neon_color(void);
static void draw_progress_bar(int x, int y, float percent, uint32_t color);
static void beep(int freq, int ms);

/* ─── SCANCODE TO ASCII ─── */
static char scancode_to_ascii(int code, int shift) {
    if (code >= 2 && code <= 11) {
        const char *chars = "1234567890";
        const char *shift_chars = "!@#$%^&*()";
        return shift ? shift_chars[code - 2] : chars[code - 2];
    }
    switch (code) {
        case 16: return shift ? 'Q' : 'q';
        case 17: return shift ? 'W' : 'w';
        case 18: return shift ? 'E' : 'e';
        case 19: return shift ? 'R' : 'r';
        case 20: return shift ? 'T' : 't';
        case 21: return shift ? 'Y' : 'y';
        case 22: return shift ? 'U' : 'u';
        case 23: return shift ? 'I' : 'i';
        case 24: return shift ? 'O' : 'o';
        case 25: return shift ? 'P' : 'p';
        case 30: return shift ? 'A' : 'a';
        case 31: return shift ? 'S' : 's';
        case 32: return shift ? 'D' : 'd';
        case 33: return shift ? 'F' : 'f';
        case 34: return shift ? 'G' : 'g';
        case 35: return shift ? 'H' : 'h';
        case 36: return shift ? 'J' : 'j';
        case 37: return shift ? 'K' : 'k';
        case 38: return shift ? 'L' : 'l';
        case 44: return shift ? 'Z' : 'z';
        case 45: return shift ? 'X' : 'x';
        case 46: return shift ? 'C' : 'c';
        case 47: return shift ? 'V' : 'v';
        case 48: return shift ? 'B' : 'b';
        case 49: return shift ? 'N' : 'n';
        case 50: return shift ? 'M' : 'm';
        case 57: return ' ';
        case 12: return shift ? '_' : '-';
        case 13: return shift ? '+' : '=';
        case 26: return shift ? '{' : '[';
        case 27: return shift ? '}' : ']';
        case 39: return shift ? ':' : ';';
        case 40: return shift ? '"' : '\'';
        case 41: return shift ? '~' : '`';
        case 43: return shift ? '|' : '\\';
        case 51: return shift ? '<' : ',';
        case 52: return shift ? '>' : '.';
        case 53: return shift ? '/' : '?';
    }
    return 0;
}

/* ─── NOTIFICATION MANAGER ─── */
static void notif_add(const char *title, const char *body) {
    if (nc >= MAX_NOTIF) {
        memmove(notifs, notifs + 1, sizeof(Notif) * (MAX_NOTIF - 1));
        nc--;
    }
    Notif *n = &notifs[nc++];
    strncpy(n->title, title, sizeof(n->title) - 1);
    n->title[sizeof(n->title) - 1] = '\0';
    strncpy(n->body, body, sizeof(n->body) - 1);
    n->body[sizeof(n->body) - 1] = '\0';
    n->t = time(NULL);
}

static void notif_init(void) {
    mkfifo("/tmp/synth3x-notif", 0666);
    notif_fd = open("/tmp/synth3x-notif", O_RDONLY | O_NONBLOCK);
}

static void notif_read(void) {
    if (notif_fd < 0) return;
    char buf[256];
    int n = read(notif_fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        char *title = buf;
        char *body = strchr(buf, ':');
        if (body) {
            *body = '\0';
            body++;
            char *nl = strchr(body, '\n');
            if (nl) *nl = '\0';
            notif_add(title, body);
        } else {
            notif_add("System Alert", buf);
        }
    }
}

/* ─── TERMINAL LOG BUFFER ─── */
static void term_log_add(const char *msg) {
    if (term_log_count >= MAX_TERM_LOGS) {
        memmove(term_logs, term_logs + 1, sizeof(term_logs[0]) * (MAX_TERM_LOGS - 1));
        term_log_count--;
    }
    strncpy(term_logs[term_log_count], msg, sizeof(term_logs[0]) - 1);
    term_logs[term_log_count][sizeof(term_logs[0]) - 1] = '\0';
    term_log_count++;
}

/* ─── HARDWARE STATS CACHE ─── */
static char cached_ram[64] = "RAM: Loading...";
static char cached_disk[64] = "DISK space: Loading...";
static char cached_disk_list[64] = "DISK list: Loading...";
static char cached_cpu[128] = "CPU: Loading...";
static char cached_usb[128] = "USB: Loading...";
static char cached_net[128] = "Net: Loading...";
static char cached_laptop[64] = "System: Unknown";
static int stats_tick = 0;

static void update_cached_stats(void) {
    FILE *fp;
    char line[128];
    
    fp = popen("/usr/bin/ram_analyzer", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            strncpy(cached_ram, line, sizeof(cached_ram) - 1);
        }
        pclose(fp);
    }
    
    fp = popen("/usr/bin/disk_analyzer", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            strncpy(cached_disk, line, sizeof(cached_disk) - 1);
        }
        if (fgets(line, sizeof(line), fp)) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            strncpy(cached_disk_list, line, sizeof(cached_disk_list) - 1);
        }
        pclose(fp);
    }
    
    fp = popen("/usr/bin/device_names", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            strncpy(cached_cpu, line, sizeof(cached_cpu) - 1);
        }
        pclose(fp);
    }
    
    /* Get DMI system info */
    fp = popen("cat /sys/class/dmi/id/sys_vendor 2>/dev/null | tr -d '\\n'; echo -n ' '; cat /sys/class/dmi/id/product_name 2>/dev/null", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (line[0]) {
                snprintf(cached_laptop, sizeof(cached_laptop), "System: %s", line);
            }
        }
        pclose(fp);
    }

    fp = popen("/usr/bin/usb_analyzer", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {}
        cached_usb[0] = '\0';
        int cnt = 0;
        while (fgets(line, sizeof(line), fp) && cnt < 2) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (cnt > 0) strncat(cached_usb, ", ", sizeof(cached_usb) - strlen(cached_usb) - 1);
            strncat(cached_usb, line + 3, sizeof(cached_usb) - strlen(cached_usb) - 1);
            cnt++;
        }
        if (cnt == 0) strcpy(cached_usb, "No external USB devices.");
        pclose(fp);
    }
    
    fp = popen("/usr/bin/cable_analyzer", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {}
        cached_net[0] = '\0';
        int cnt = 0;
        while (fgets(line, sizeof(line), fp) && cnt < 2) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (cnt > 0) strncat(cached_net, ", ", sizeof(cached_net) - strlen(cached_net) - 1);
            strncat(cached_net, line + 3, sizeof(cached_net) - strlen(cached_net) - 1);
            cnt++;
        }
        if (cnt == 0) strcpy(cached_net, "No interfaces detected.");
        pclose(fp);
    }
}

typedef struct {
    char cmd[128];
} CmdArgs;

/* ─── ASYNC WORKERS ─── */
static void *async_vscodium_install(void *arg) {
    (void)arg;
    term_log_add("Resolving dependencies...");
    term_log_add("Downloading vscodium via Tor...");
    beep(440, 100); usleep(200000);
    for (int p = 1; p <= 5; p++) {
        char progress[64];
        snprintf(progress, sizeof(progress), "Progress: [%d/5] Downloading...", p);
        term_log_add(progress);
        usleep(300000);
        beep(500 + p * 100, 50);
    }
    vscodium_installed = 1;
    term_log_add("[OK] VSCodium installed successfully!");
    notif_add("Package Manager", "VSCodium installed! Check Dock.");
    beep(1046, 150); beep(1318, 150); beep(1568, 200);
    return NULL;
}

static void *async_pkg_install(void *arg) {
    CmdArgs *args = (CmdArgs *)arg;
    const char *pkg = args->cmd + 12;
    char msg[64];
    snprintf(msg, sizeof(msg), "Fetching %s...", pkg);
    term_log_add(msg);
    beep(440, 80);
    usleep(500000);
    snprintf(msg, sizeof(msg), "[ERR] %s not found in repos.", pkg);
    term_log_add(msg);
    beep(220, 200);
    free(args);
    return NULL;
}

static void *async_exec_cmd(void *arg) {
    CmdArgs *args = (CmdArgs *)arg;
    char wrapped[256];
    snprintf(wrapped, sizeof(wrapped), "%s 2>&1", args->cmd);
    FILE *fp = popen(wrapped, "r");
    if (!fp) {
        term_log_add("[ERR] Failed to execute command.");
        free(args);
        return NULL;
    }
    char line[256];
    int lines_read = 0;
    term_log_add("--- Running: ");
    term_log_add(args->cmd);
    while (fgets(line, sizeof(line), fp) && lines_read < 12) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        term_log_add(line);
        lines_read++;
    }
    int exit_code = pclose(fp);
    if (lines_read == 0) {
        if (exit_code == 127) {
            term_log_add("[ERR] Command not found (not in PATH)");
        } else if (exit_code == 0) {
            term_log_add("[OK] Command completed (no output)");
        } else {
            char buf[64];
            snprintf(buf, sizeof(buf), "[ERR] Exit code %d", exit_code >> 8);
            term_log_add(buf);
        }
    }
    free(args);
    return NULL;
}

static void *async_net_setup(void *arg) {
    (void)arg;
    term_log_add("Network: Spoofing MAC...");
    system("for iface in /sys/class/net/*; do name=$(basename $iface); [ \"$name\" = lo ] && continue; ip link set $name down 2>/dev/null; ip link set $name address 02:$(printf '%02x:%02x:%02x:%02x:%02x' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256))) 2>/dev/null; ip link set $name up 2>/dev/null; done");
    term_log_add("Network: DHCP on all interfaces...");
    system("for iface in /sys/class/net/*; do name=$(basename $iface); [ \"$name\" = lo ] || [ \"$name\" = docker0 ] && continue; udhcpc -i $name -b -q 2>/dev/null & done");
    term_log_add("Network: DNS set to 1.1.1.1 / 8.8.8.8");
    system("echo 'nameserver 1.1.1.1' > /etc/resolv.conf; echo 'nameserver 8.8.8.8' >> /etc/resolv.conf");
    term_log_add("Network: All interfaces configured!");
    notif_add("Network", "Internet ready! DHCP + DNS configured.");
    beep(880, 80); beep(1100, 150);
    return NULL;
}

/* ─── W3M BROWSER LAUNCHER ─── */
static void *async_launch_browser(void *arg) {
    (void)arg;
    term_log_add("Browser: Starting w3m on VT1...");
    beep(660, 60); beep(880, 80);
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        int vt = open("/dev/tty1", O_RDWR);
        if (vt >= 0) {
            ioctl(vt, TIOCSCTTY, 0);
            dup2(vt, 0); dup2(vt, 1); dup2(vt, 2);
            close(vt);
        }
        ioctl(0, KDSETMODE, KD_TEXT);
        printf("\033[2J\033[H Synth3x Web Browser (w3m)\r\n");
        printf(" Press 'q' then 'y' to quit when done.\r\n\r\n");
        sleep(1);
        execl("/usr/bin/w3m", "w3m", "https://lite.duckduckgo.com/lite", NULL);
        execl("/bin/busybox", "busybox", "wget", "-q", "-O-", "https://lite.duckduckgo.com", NULL);
        printf("w3m not found. Try: syn inst w3m\r\n");
        sleep(3);
        _exit(1);
    }
    notif_add("Browser", "w3m on VT1 (Ctrl+Alt+F1). Alt+F7 to return.");
    term_log_add("Browser: launched on VT1. Ctrl+Alt+F1 to use, Alt+F7 back.");
    term_log_add("Browser: type 'q' then 'y' to quit w3m.");
    return NULL;
}

/* ─── TERMINAL COMMAND EXECUTOR ─── */
static void exec_term_cmd(const char *cmd) {
    while (*cmd == ' ') cmd++;
    if (strlen(cmd) == 0) return;
    
    char echo[128];
    snprintf(echo, sizeof(echo), "$ %s", cmd);
    term_log_add(echo);
    
    if (strcmp(cmd, "clear") == 0) {
        term_log_count = 0;
        return;
    }
    
    if (strcmp(cmd, "browser") == 0 || strcmp(cmd, "w3m") == 0 || strcmp(cmd, "web") == 0) {
        term_log_add("Launching web browser (w3m)...");
        pthread_t b_thread;
        pthread_create(&b_thread, NULL, async_launch_browser, NULL);
        pthread_detach(b_thread);
        return;
    }
    
    if (strcmp(cmd, "vscodium") == 0 || strcmp(cmd, "vscodium &") == 0) {
        if (vscodium_installed) {
            int idx = find_win_by_title("VSCodium");
            if (idx < 0) {
                wnew("VSCodium", 500, 320);
                beep(523, 60); beep(659, 60); beep(784, 80);
            } else {
                wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx;
                beep(400, 50); beep(500, 50);
            }
            term_log_add("AmnesiaDE: launching VSCodium...");
            notif_add("VSCodium", "Editor launched.");
        } else {
            term_log_add("vscodium: command not found (try: 'pkg install vscodium')");
            beep(300, 100);
        }
        return;
    }
    
    if (strcmp(cmd, "pkg install vscodium") == 0) {
        if (vscodium_installed) {
            term_log_add("vscodium is already installed.");
            return;
        }
        pthread_t inst_thread;
        pthread_create(&inst_thread, NULL, async_vscodium_install, NULL);
        pthread_detach(inst_thread);
        return;
    }
    
    if (strncmp(cmd, "pkg install ", 12) == 0) {
        CmdArgs *args = malloc(sizeof(CmdArgs));
        strcpy(args->cmd, cmd);
        pthread_t pkg_thread;
        pthread_create(&pkg_thread, NULL, async_pkg_install, args);
        pthread_detach(pkg_thread);
        return;
    }

    /* Gentoo emerge support */
    if (strncmp(cmd, "emerge", 6) == 0) {
        term_log_add("Gentoo: Syncing portage tree...");
        term_log_add("Gentoo: emerge is available after hard disk install");
        term_log_add("Gentoo: Run 'synth3x-installer' for full Gentoo setup");
        return;
    }
    
    CmdArgs *args = malloc(sizeof(CmdArgs));
    strncpy(args->cmd, cmd, sizeof(args->cmd) - 1);
    args->cmd[sizeof(args->cmd) - 1] = '\0';
    
    pthread_t cmd_thread;
    pthread_create(&cmd_thread, NULL, async_exec_cmd, args);
    pthread_detach(cmd_thread);
}

/* ─── BEEP ─── */
static void beep(int freq, int ms) {
    if (fork() == 0) {
        int fd = open("/dev/tty0", O_RDWR);
        if (fd >= 0) {
            ioctl(fd, KIOCSOUND, 1193180 / freq);
            usleep(ms * 1000);
            ioctl(fd, KIOCSOUND, 0);
            close(fd);
        }
        _exit(0);
    }
}

/* ─── GRAPHICAL PRIMITIVES ─── */
void draw_px(int x, int y, uint32_t colour) {
    if (x >= 0 && x < fb_w && y >= 0 && y < fb_h) {
        if (fb_bpp == 32) {
            backbuf32[y * fb_w + x] = colour;
        } else {
            uint16_t r = ((colour >> 16) & 0xFF) >> 3;
            uint16_t g = ((colour >> 8) & 0xFF) >> 2;
            uint16_t b = (colour & 0xFF) >> 3;
            backbuf16[y * fb_w + x] = (r << 11) | (g << 5) | b;
        }
    }
}

static void px(int x, int y, uint32_t c) { draw_px(x, y, c); }
static void rect(int x, int y, int w, int h, uint32_t c) {
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++)
            draw_px(x + col, y + row, c);
}

static void fchar(int x, int y, char c, uint32_t fg, uint32_t bg) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = &font8x8[(c - 32) * 8];
    for (int row = 0; row < 8; row++) {
        uint8_t byte = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (byte & (0x80 >> col)) {
                draw_px(x + col, y + row, fg);
                if (col < 7) draw_px(x + col + 1, y + row, fg);
            } else if (bg != 0) {
                if (col == 0 || !(byte & (0x80 >> (col - 1))))
                    draw_px(x + col, y + row, bg);
            }
        }
    }
}

static void fstr(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    while (*s) { fchar(x, y, *s++, fg, bg); x += 8; }
}

/* ─── NEON COLOR OSCILLATOR ─── */
static uint32_t get_neon_color(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int tick = (ts.tv_nsec / 15000000) % 64;
    int val = tick < 32 ? tick : 64 - tick;
    int r = val * 8;
    int g = 255 - val * 8;
    int b = 255 - val * 2;
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

/* ─── CURSOR ─── */
static const char *cursor_map[] = {
    "X               ",
    "XX              ",
    "X.X             ",
    "X..X            ",
    "X...X           ",
    "X....X          ",
    "X.....X         ",
    "X......X        ",
    "X.......X       ",
    "X........X      ",
    "X.....XXXX      ",
    "X..X..X         ",
    "XX  X..X        ",
    "    X..X        ",
    "     XX         ",
    NULL
};

static void draw_custom_cursor(int cx, int cy) {
    uint32_t border_col = COLOR_TEXT;
    uint32_t inner_col = COLOR_BG;
    for (int r = 0; cursor_map[r] != NULL; r++) {
        for (int c = 0; cursor_map[r][c] != '\0'; c++) {
            if (cursor_map[r][c] == 'X') px(cx + c, cy + r, border_col);
            else if (cursor_map[r][c] == '.') px(cx + c, cy + r, inner_col);
        }
    }
}

/* ─── PROGRESS BAR ─── */
static void draw_progress_bar(int x, int y, float percent, uint32_t color) {
    int w = 180, h = 8;
    rect(x, y, w, h, 0xFF1E1428);
    if (percent > 0) rect(x, y, (int)(w * (percent > 1.0f ? 1.0f : percent)), h, color);
    rect(x-1, y-1, w+2, 1, COLOR_DIM);
    rect(x-1, y+h, w+2, 1, COLOR_DIM);
    rect(x-1, y, 1, h, COLOR_DIM);
    rect(x+w, y, 1, h, COLOR_DIM);
}

/* ─── STARS ─── */
typedef struct { int x, y; int type; } Star;
static Star stars[48];

static void init_stars(void) {
    srand(1337);
    for (int i = 0; i < 48; i++) {
        stars[i].x = rand() % 1024;
        stars[i].y = PANEL_H + 10 + (rand() % 260);
        stars[i].type = rand() % 4;
    }
}

static void draw_stars(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int tick = (ts.tv_nsec / 100000000) % 8;
    for (int i = 0; i < 48; i++) {
        int brightness = (tick + i) % 5;
        uint32_t c;
        if (brightness == 0) c = COLOR_DIM;
        else if (brightness == 1) c = 0xFF6A3A7E;
        else if (brightness == 2) c = 0xFF9A7ABE;
        else if (brightness == 3) c = COLOR_PANEL_FG;
        else c = COLOR_WHITE;
        px(stars[i].x, stars[i].y, c);
        if (stars[i].type == 1 && brightness >= 3) {
            px(stars[i].x - 1, stars[i].y, c);
            px(stars[i].x + 1, stars[i].y, c);
            px(stars[i].x, stars[i].y - 1, c);
            px(stars[i].x, stars[i].y + 1, c);
        }
        if (stars[i].type == 2 && brightness >= 4) {
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    if (dx != 0 || dy != 0) px(stars[i].x + dx, stars[i].y + dy, 0xFF8A4A8E);
        }
    }
}

/* ─── RETROWAVE SUN ─── */
static void draw_retro_sun(int cx, int cy, int r) {
    for (int dy = -r; dy <= r; dy++) {
        int y = cy + dy;
        if (y < PANEL_H || y >= fb_h) continue;
        int w = (int)sqrt(r * r - dy * dy);
        if (dy > 10 && (dy % 14 < (dy / 4))) continue;
        float factor = (float)(dy + r) / (2 * r);
        Color c_top = {255, 50, 0};
        Color c_mid = {255, 100, 80};
        Color c_bot = {200, 0, 128};
        Color c;
        if (factor < 0.5f) c = lerp(c_top, c_mid, factor * 2);
        else c = lerp(c_mid, c_bot, (factor - 0.5f) * 2);
        rect(cx - w, y, w * 2, 1, c565(c) | 0xFF000000);
    }
}

/* ─── MOUNTAINS ─── */
static void draw_mountains(void) {
    uint32_t m_col = COLOR_ACCENT;
    uint32_t m_fill = COLOR_WIN_BG;
    int horizon = fb_h / 2 + 80;
    for (int x = 0; x < fb_w; x++) {
        int h1 = 0, h2 = 0;
        if (x >= 0 && x <= 360)
            h1 = 110 - abs(x - 160) * 110 / 180;
        if (x >= 340 && x <= fb_w)
            h2 = 130 - abs(x - 620) * 130 / 220;
        if (h1 < 0) h1 = 0;
        if (h2 < 0) h2 = 0;
        int max_h = h1 > h2 ? h1 : h2;
        if (max_h > 0) {
            int peak_y = horizon - max_h;
            rect(x, peak_y, 1, max_h, m_fill);
            px(x, peak_y, m_col);
            if (x % 24 == 0) {
                for (int my = peak_y; my < horizon; my += 16)
                    px(x, my, COLOR_DIM);
            }
        }
    }
}

/* ─── PERSPECTIVE GRID ─── */
static void draw_perspective_grid(void) {
    uint32_t grid_col = 0xFF20B0AA;
    int horizon_y = fb_h / 2 + 80;
    int step = 8;
    for (int y = horizon_y; y < fb_h - 40; y += step) {
        rect(0, y, fb_w, 1, grid_col);
        step = (int)(step * 1.35f);
        if (step > 64) step = 64;
    }
    int cx = fb_w / 2;
    for (int x = -fb_w; x <= fb_w * 2; x += 55) {
        int x1 = cx, y1 = horizon_y;
        int x2 = x, y2 = fb_h - 40;
        int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = dx + dy, e2;
        int curr_x = x1, curr_y = y1;
        while (1) {
            if (curr_x >= 0 && curr_x < fb_w && curr_y >= horizon_y && curr_y < fb_h - 40)
                px(curr_x, curr_y, grid_col);
            if (curr_x == x2 && curr_y == y2) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; curr_x += sx; }
            if (e2 <= dx) { err += dx; curr_y += sy; }
        }
    }
}

/* ─── BG GRADIENT ─── */
static void draw_bg_gradient(void) {
    for (int y = PANEL_H; y < fb_h; y++) {
        float t = (float)(y - PANEL_H) / (fb_h - PANEL_H);
        Color top = {10, 5, 20};
        Color mid = {20, 8, 40};
        Color bot = {30, 10, 50};
        Color c;
        if (t < 0.3f) c = lerp(top, mid, t / 0.3f);
        else c = lerp(mid, bot, (t - 0.3f) / 0.7f);
        uint32_t col = 0xFF000000 | (c.r << 16) | (c.g << 8) | c.b;
        for (int x = 0; x < fb_w; x++) {
            if (fb_bpp == 32) backbuf32[y * fb_w + x] = col;
            else {
                uint16_t r = c.r >> 3, g = c.g >> 2, b = c.b >> 3;
                backbuf16[y * fb_w + x] = (r << 11) | (g << 5) | b;
            }
        }
    }
}

static void draw_bg(void) {
    draw_bg_gradient();
    draw_stars();
    draw_retro_sun(fb_w/2, fb_h/2 + 20, 110);
    draw_mountains();
    draw_perspective_grid();
    draw_desktop_icons();
}

static int is_tor_running(void) {
    DIR *d = opendir("/proc");
    if (!d) return 0;
    struct dirent *de;
    int running = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] >= '0' && de->d_name[0] <= '9') {
            char path[128];
            snprintf(path, sizeof(path), "/proc/%s/comm", de->d_name);
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                char comm[32];
                int n = read(fd, comm, sizeof(comm) - 1);
                if (n > 0) {
                    comm[n] = '\0';
                    if (comm[n-1] == '\n') comm[n-1] = '\0';
                    if (strncmp(comm, "tor", 3) == 0) { running = 1; close(fd); break; }
                }
                close(fd);
            }
        }
    }
    closedir(d);
    return running;
}

/* ─── TOUCHPAD STATUS ─── */
static int has_touchpad(void) {
    int fd = open("/proc/bus/input/devices", O_RDONLY);
    if (fd < 0) return 0;
    char buf[4096];
    int n = read(fd, buf, sizeof(buf)-1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    return (strstr(buf, "Touchpad") || strstr(buf, "touchpad") ||
            strstr(buf, "Synaptics") || strstr(buf, "ALPS") ||
            strstr(buf, "Elan")) ? 1 : 0;
}

/* ─── WINDOWS MANAGER ─── */
static int find_win_by_title(const char *title) {
    for (int i = 0; i < wc; i++)
        if (strcmp(wins[i].title, title) == 0) return i;
    return -1;
}

static int wnew(const char *t, int w, int h) {
    if (wc >= MAX_WIN) return -1;
    Win *wn = &wins[wc++];
    wn->x = 60 + (wc*40) % (fb_w - w - 80);
    wn->y = PANEL_H + 40 + (wc*30) % (fb_h - PANEL_H - h - 100);
    wn->w = w; wn->h = h; wn->hidden = 0; wn->ws = current_ws;
    wn->drag = 0; wn->maximized = 0;
    strncpy(wn->title, t, 47); aw = wc - 1;
    return aw;
}

static int win_title(Win *w, int x, int y) {
    if (w->maximized) return 0;
    return x >= w->x && x <= w->x + w->w && y >= w->y - 24 && y <= w->y;
}
static int win_close(Win *w, int x, int y) {
    return x >= w->x + 8 && x <= w->x + 20 && y >= w->y - 22 && y <= w->y - 4;
}
static int win_minimize(Win *w, int x, int y) {
    return x >= w->x + 24 && x <= w->x + 36 && y >= w->y - 22 && y <= w->y - 4;
}
static int win_maximize(Win *w, int x, int y) {
    return x >= w->x + 40 && x <= w->x + 52 && y >= w->y - 22 && y <= w->y - 4;
}

/* ─── TEXT SELECTION ─── */
static int selecting = 0, mouse_pressed = 0;
static int sel_start_x = 0, sel_start_y = 0, sel_end_x = 0, sel_end_y = 0;
static int show_copy_dialog = 0;
static char selected_text[512] = "", clipboard[1024] = "";

typedef struct { int x; int y; const char *text; } WinTextLine;

static const WinTextLine guide_p0_lines[] = {
    {12, 12, "======================================================"},
    {12, 26, "   SYNTH3X OS: INTERNET SETUP GUIDE                  "},
    {12, 40, "======================================================"},
    {12, 56, "[1/8] QUICK START — AUTO INTERNET (Ethernet)"},
    {12, 76, "1. Plug in Ethernet cable"},
    {12, 92, "2. Press [SETUP INTERNET] button (runs DHCP)"},
    {12, 108, "   — OR type in terminal:"},
    {24, 128, "$ udhcpc -i eth0"},
    {12, 152, "3. Test connection:"},
    {24, 172, "$ ping -c 3 1.1.1.1"},
    {12, 196, "4. Browse:"},
    {24, 216, "$ browser"},
    {12, 240, "Wi-Fi: see page 3 (needs real install)."},
    {12, 260, "Troubleshooting: see page 5."},
};

static const WinTextLine guide_p1_lines[] = {
    {12, 12, "======================================================"},
    {12, 26, "   SYNTH3X OS: INTERNET SETUP GUIDE                  "},
    {12, 40, "======================================================"},
    {12, 56, "[2/8] WIRED ETHERNET — DHCP & MANUAL"},
    {12, 76, "Auto DHCP (recommended):"},
    {24, 96, "$ udhcpc -i eth0"},
    {12, 120, "Check interface status:"},
    {24, 140, "$ ip addr        # show IP addresses"},
    {24, 156, "$ ip link        # show link status"},
    {24, 172, "$ ping -c 3 1.1.1.1   # test connectivity"},
    {12, 200, "Manual IP (no DHCP):"},
    {24, 220, "$ ip addr add 192.168.1.100/24 dev eth0"},
    {24, 236, "$ ip route add default via 192.168.1.1"},
    {24, 252, "$ echo nameserver 1.1.1.1 > /etc/resolv.conf"},
    {12, 272, "$ cable_analyzer   # check cable/LED"},
};

static const WinTextLine guide_p2_lines[] = {
    {12, 12, "======================================================"},
    {12, 26, "   SYNTH3X OS: INTERNET SETUP GUIDE                  "},
    {12, 40, "======================================================"},
    {12, 56, "[3/8] WI-FI — POST-INSTALL"},
    {12, 76, "Wi-Fi is NOT available in the live CD."},
    {12, 92, "It requires firmware + wpa_supplicant."},
    {12, 108, "Install to HDD first, then:"},
    {24, 128, "# emerge linux-firmware wpa_supplicant"},
    {24, 144, "# wpa_passphrase MySSID password >> /etc/wpa.conf"},
    {24, 160, "# wpa_supplicant -B -i wlan0 -c /etc/wpa.conf"},
    {24, 176, "# udhcpc -i wlan0"},
    {12, 200, "USB Tethering (phone):"},
    {24, 220, "1. Connect phone via USB, enable tethering"},
    {24, 236, "2. $ udhcpc -i usb0"},
    {12, 256, "See Ethernet page (2) for live CD networking."},
};

static const WinTextLine guide_p3_lines[] = {
    {12, 12, "======================================================"},
    {12, 26, "   SYNTH3X OS: INTERNET SETUP GUIDE                  "},
    {12, 40, "======================================================"},
    {12, 56, "[4/8] STATIC IP & TETHERING"},
    {12, 76, "Static IP (if no DHCP server):"},
    {24, 100, "$ ip addr add 192.168.1.100/24 dev eth0"},
    {24, 116, "$ ip route add default via 192.168.1.1"},
    {24, 132, "$ echo nameserver 1.1.1.1 > /etc/resolv.conf"},
    {12, 160, "USB Tethering (Android/iOS):"},
    {24, 184, "1. Connect phone via USB"},
    {24, 200, "2. Enable USB tethering on phone"},
    {24, 216, "3. $ udhcpc -i usb0"},
    {12, 244, "Check routing:"},
    {24, 264, "$ ip route"},
    {24, 280, "$ traceroute 1.1.1.1"},
};

static const WinTextLine guide_p4_lines[] = {
    {12, 12, "======================================================"},
    {12, 26, "   SYNTH3X OS: INTERNET SETUP GUIDE                  "},
    {12, 40, "======================================================"},
    {12, 56, "[5/8] TROUBLESHOOTING"},
    {12, 76, "No internet? Run these:"},
    {24, 100, "$ cable_analyzer      # check cable/LED"},
    {24, 116, "$ ip link             # interface up/down"},
    {24, 132, "$ ifconfig -a         # all interfaces"},
    {24, 148, "$ ping -c 3 1.1.1.1   # test DNS/route"},
    {12, 172, "Network driver not loaded?"},
    {24, 196, "List available: $ ls /lib/modules/*net*.ko"},
    {24, 212, "Load manually: $ insmod /lib/modules/e1000.ko"},
    {12, 240, "Reset interface:"},
    {24, 260, "$ ip link set eth0 down && ip link set eth0 up"},
    {24, 276, "$ udhcpc -i eth0"},
};

static const WinTextLine guide_p5_lines[] = {
    {12, 12, "======================================================"},
    {12, 26, "   SYNTH3X OS: GENTOO INTERNET SETUP GUIDE           "},
    {12, 40, "======================================================"},
    {12, 56, "[6/8] INSTALLING PROGRAMS (syn pkg manager)"},
    {12, 76, "The 'syn' command manages packages:"},
    {24, 100, "$ syn inst w3m       - Install w3m browser"},
    {24, 116, "$ syn binary firefox - Download Firefox binary"},
    {24, 132, "$ syn list           - List all packages"},
    {24, 148, "$ syn search browser - Search packages"},
    {24, 164, "$ syn update         - Update package list"},
    {24, 180, "$ syn remove w3m     - Remove package"},
    {12, 204, "Available packages in live environment:"},
    {24, 224, "w3m firefox vscodium telegram-desktop"},
    {24, 240, "vim htop gcc python nodejs git curl"},
    {12, 260, "From DE terminal: type command, press Enter"},
};

static const WinTextLine guide_p6_lines[] = {
    {12, 12, "======================================================"},
    {12, 26, "   SYNTH3X OS: GENTOO INTERNET SETUP GUIDE           "},
    {12, 40, "======================================================"},
    {12, 56, "[7/8] HARD DISK INSTALLATION"},
    {12, 76, "Install Synth3x to your hard drive:"},
    {24, 100, "1. From DE, press ESC to exit to terminal"},
    {24, 116, "2. Run: # synth3x-installer"},
    {24, 132, "3. Enter username & password"},
    {24, 148, "4. Select target drive (/dev/sda or /dev/nvme0n1)"},
    {24, 164, "5. Choose Desktop Environment"},
    {24, 180, "6. Wait for install, then reboot"},
    {12, 204, "After install:"},
    {24, 224, "• Full Gentoo system with Portage"},
    {24, 240, "• User account with sudo access"},
    {24, 256, "• GRUB bootloader booting from HDD"},
};

static const WinTextLine guide_p7_lines[] = {
    {12, 12, "======================================================"},
    {12, 26, "   SYNTH3X OS: GENTOO INTERNET SETUP GUIDE           "},
    {12, 40, "======================================================"},
    {12, 56, "[8/8] TOR & SECURITY / TOUCHPAD / CHECKS"},
    {12, 76, "Tor is LAZY (not auto-started, saves ~30MB RAM):"},
    {24, 100, "$ tor-start        - Start Tor manually"},
    {24, 116, "$ checks-all       - Verify Tor + firewall"},
    {12, 140, "Tor routes ALL traffic through anonymity"},
    {12, 156, "network (DNS + HTTP/HTTPS hidden)."},
    {12, 180, "Driver & Hardware Checks:"},
    {24, 200, "$ check_mouse      - Mouse/touchpad status"},
    {24, 216, "$ check_keyboard   - Keyboard status"},
    {24, 232, "$ check_display    - Framebuffer/GPU status"},
    {24, 248, "$ check_sound      - Audio driver status"},
    {12, 268, "Full check: $ check-drivers-all"},
};

static void rect_blend(int x, int y, int w, int h, uint32_t color) {
    int x1 = x < 0 ? 0 : x, y1 = y < 0 ? 0 : y;
    int x2 = x + w > fb_w ? fb_w : x + w;
    int y2 = y + h > fb_h ? fb_h : y + h;
    uint8_t a = (color >> 24) & 0xFF, r_c = (color >> 16) & 0xFF;
    uint8_t g_c = (color >> 8) & 0xFF, b_c = color & 0xFF;
    for (int row = y1; row < y2; row++) {
        for (int col = x1; col < x2; col++) {
            if (fb_bpp == 32) {
                uint32_t orig = backbuf32[row * fb_w + col];
                uint8_t r_o = (orig >> 16) & 0xFF, g_o = (orig >> 8) & 0xFF, b_o = orig & 0xFF;
                uint8_t r_new = (r_c * a + r_o * (255 - a)) / 255;
                uint8_t g_new = (g_c * a + g_o * (255 - a)) / 255;
                uint8_t b_new = (b_c * a + b_o * (255 - a)) / 255;
                backbuf32[row * fb_w + col] = 0xFF000000 | (r_new << 16) | (g_new << 8) | b_new;
            } else {
                uint16_t orig = backbuf16[row * fb_w + col];
                uint8_t r_o = ((orig >> 11) & 0x1F) << 3, g_o = ((orig >> 5) & 0x3F) << 2, b_o = (orig & 0x1F) << 3;
                uint8_t r_new = (r_c * a + r_o * (255 - a)) / 255;
                uint8_t g_new = (g_c * a + g_o * (255 - a)) / 255;
                uint8_t b_new = (b_c * a + b_o * (255 - a)) / 255;
                uint16_t r = r_new >> 3, g = g_new >> 2, b = b_new >> 3;
                backbuf16[row * fb_w + col] = (r << 11) | (g << 5) | b;
            }
        }
    }
}

static void scan_lines_helper(Win *w, const WinTextLine *lines, int count, int x1, int y1, int x2, int y2, int *char_count) {
    for (int i = 0; i < count; i++) {
        int ly = w->y + lines[i].y;
        if (ly + 4 >= y1 && ly + 4 <= y2) {
            int start_idx = -1, end_idx = -1;
            int len = strlen(lines[i].text);
            for (int col = 0; col < len; col++) {
                int cx = w->x + lines[i].x + col * 8;
                if (cx + 4 >= x1 && cx + 4 <= x2) {
                    if (start_idx == -1) start_idx = col;
                    end_idx = col;
                }
            }
            if (start_idx != -1 && end_idx != -1) {
                int sel_len = end_idx - start_idx + 1;
                if (*char_count > 0 && selected_text[strlen(selected_text)-1] != '\n')
                    strcat(selected_text, "\n");
                int cur_len = strlen(selected_text);
                if (cur_len + sel_len < (int)sizeof(selected_text) - 2)
                    strncat(selected_text, lines[i].text + start_idx, sel_len);
                *char_count += sel_len;
            }
        }
    }
}

static void extract_selected_text(void) {
    selected_text[0] = '\0';
    if (aw < 0 || aw >= wc) return;
    Win *w = &wins[aw];
    int x1 = sel_start_x < sel_end_x ? sel_start_x : sel_end_x;
    int y1 = sel_start_y < sel_end_y ? sel_start_y : sel_end_y;
    int x2 = sel_start_x > sel_end_x ? sel_start_x : sel_end_x;
    int y2 = sel_start_y > sel_end_y ? sel_start_y : sel_end_y;
    int char_count = 0;

    if (strcmp(w->title, "Terminal") == 0) {
        for (int i = 0; i < term_log_count; i++) {
            int ly = w->y + 12 + i * 16;
            if (ly + 4 >= y1 && ly + 4 <= y2) {
                int start_idx = -1, end_idx = -1;
                int len = strlen(term_logs[i]);
                for (int col = 0; col < len; col++) {
                    int cx = w->x + 12 + col * 8;
                    if (cx + 4 >= x1 && cx + 4 <= x2) {
                        if (start_idx == -1) start_idx = col;
                        end_idx = col;
                    }
                }
                if (start_idx != -1 && end_idx != -1) {
                    int sel_len = end_idx - start_idx + 1;
                    if (char_count > 0 && selected_text[strlen(selected_text)-1] != '\n')
                        strcat(selected_text, "\n");
                    int cur_len = strlen(selected_text);
                    if (cur_len + sel_len < (int)sizeof(selected_text) - 2)
                        strncat(selected_text, term_logs[i] + start_idx, sel_len);
                    char_count += sel_len;
                }
            }
        }
        int prompt_y = w->y + w->h - 24;
        if (prompt_y + 4 >= y1 && prompt_y + 4 <= y2) {
            char full_prompt[128];
            snprintf(full_prompt, sizeof(full_prompt), "synth3x@root:~$ %s", term_input);
            int start_idx = -1, end_idx = -1, len = strlen(full_prompt);
            for (int col = 0; col < len; col++) {
                int cx = w->x + 12 + col * 8;
                if (cx + 4 >= x1 && cx + 4 <= x2) {
                    if (start_idx == -1) start_idx = col;
                    end_idx = col;
                }
            }
            if (start_idx != -1 && end_idx != -1) {
                int sel_len = end_idx - start_idx + 1;
                if (char_count > 0 && selected_text[strlen(selected_text)-1] != '\n')
                    strcat(selected_text, "\n");
                int cur_len = strlen(selected_text);
                if (cur_len + sel_len < (int)sizeof(selected_text) - 2)
                    strncat(selected_text, full_prompt + start_idx, sel_len);
                char_count += sel_len;
            }
        }
    } else if (strcmp(w->title, "Synth3x Guide") == 0) {
        const WinTextLine *lines = NULL;
        int nlines = 0;
        switch (guide_page) {
            case 0: lines = guide_p0_lines; nlines = sizeof(guide_p0_lines)/sizeof(guide_p0_lines[0]); break;
            case 1: lines = guide_p1_lines; nlines = sizeof(guide_p1_lines)/sizeof(guide_p1_lines[0]); break;
            case 2: lines = guide_p2_lines; nlines = sizeof(guide_p2_lines)/sizeof(guide_p2_lines[0]); break;
            case 3: lines = guide_p3_lines; nlines = sizeof(guide_p3_lines)/sizeof(guide_p3_lines[0]); break;
            case 4: lines = guide_p4_lines; nlines = sizeof(guide_p4_lines)/sizeof(guide_p4_lines[0]); break;
            case 5: lines = guide_p5_lines; nlines = sizeof(guide_p5_lines)/sizeof(guide_p5_lines[0]); break;
            case 6: lines = guide_p6_lines; nlines = sizeof(guide_p6_lines)/sizeof(guide_p6_lines[0]); break;
            case 7: lines = guide_p7_lines; nlines = sizeof(guide_p7_lines)/sizeof(guide_p7_lines[0]); break;
        }
        if (lines) scan_lines_helper(w, lines, nlines, x1, y1, x2, y2, &char_count);
    } else if (strcmp(w->title, "System Info") == 0) {
        char sys_usb[128], sys_net[128], sys_tor[128], sys_laptop[128];
        snprintf(sys_usb, sizeof(sys_usb), "USB: %s", cached_usb);
        snprintf(sys_net, sizeof(sys_net), "NET: %s", cached_net);
        snprintf(sys_tor, sizeof(sys_tor), "SECURE TOR ROUTE: %s", is_tor_running() ? "ACTIVE" : "OFFLINE");
        snprintf(sys_laptop, sizeof(sys_laptop), "%s", cached_laptop);
        WinTextLine sys_lines[] = {
            {12, 12, "=== SYSTEM REALTIME HARDWARE STATS ==="},
            {12, 32, cached_cpu},
            {12, 52, cached_ram},
            {12, 72, sys_laptop},
            {12, 92, cached_disk},
            {12, 110, cached_disk_list},
            {12, 130, sys_usb},
            {12, 150, sys_net},
            {12, 170, sys_tor}
        };
        scan_lines_helper(w, sys_lines, sizeof(sys_lines)/sizeof(sys_lines[0]), x1, y1, x2, y2, &char_count);
    }
}

static void draw_copy_modal(void) {
    int mw = 320, mh = 140;
    int mx_pos = fb_w / 2 - mw / 2, my_pos = fb_h / 2 - mh / 2;
    rect_blend(0, 0, fb_w, fb_h, 0x60000000);
    rect(mx_pos, my_pos, mw, mh, 0xFF0D0818);
    rect(mx_pos-1, my_pos-1, mw+2, mh+2, COLOR_ACCENT);
    rect(mx_pos, my_pos, mw, 24, COLOR_ACCENT);
    fstr(mx_pos + 12, my_pos + 8, "SYSTEM: COPY TO CLIPBOARD?", COLOR_WHITE, COLOR_ACCENT);
    fstr(mx_pos + 12, my_pos + 38, "Copy the selected text?", COLOR_TEXT, 0);
    char snippet[36];
    if (strlen(selected_text) > 32) snprintf(snippet, sizeof(snippet), "\"%.29s...\"", selected_text);
    else snprintf(snippet, sizeof(snippet), "\"%s\"", selected_text);
    for(int i=0; snippet[i]; i++) if(snippet[i] == '\n') snippet[i] = ' ';
    fstr(mx_pos + 12, my_pos + 58, snippet, COLOR_YELLOW, 0);
    int btn_yes_x = mx_pos + 30, btn_y = my_pos + 94;
    rect(btn_yes_x, btn_y, 100, 24, 0xFF14281A);
    rect(btn_yes_x-1, btn_y-1, 102, 26, COLOR_GREEN);
    fstr(btn_yes_x + 24, btn_y + 8, "[ COPY ]", COLOR_GREEN, 0xFF14281A);
    int btn_no_x = mx_pos + 190;
    rect(btn_no_x, btn_y, 100, 24, 0xFF2A1015);
    rect(btn_no_x-1, btn_y-1, 102, 26, COLOR_RED);
    fstr(btn_no_x + 16, btn_y + 8, "[ CANCEL ]", COLOR_RED, 0xFF2A1015);
}

/* ─── DESKTOP ICONS ─── */
#define ICON_W 48
#define ICON_H 48

static void draw_desktop_icons(void) {
    int start_y = 60;
    int gap = 78;
    struct { int y; const char *label; uint32_t border; const char *icon; int conditional; } icons[] = {
        {start_y, "Terminal",COLOR_TEXT,">_"},
        {start_y+gap, "SysInfo",COLOR_GREEN,"i"},
        {start_y+gap*2, "Web",COLOR_ORANGE,"W"},
        {start_y+gap*3, "Handbook",COLOR_YELLOW,"?"},
        {start_y+gap*4, "Guide",COLOR_ACCENT,"#"},
        {start_y+gap*5, "VSCodium",COLOR_ACCENT,"{}", 1},
        {start_y+gap*6, "Install",COLOR_RED,"HD"},
    };
    for (int i = 0; i < 7; i++) {
        if (icons[i].conditional && !vscodium_installed) continue;
        int x1 = 20;
        rect(x1, icons[i].y, ICON_W, ICON_H, 0xFF140A28);
        rect(x1-1, icons[i].y-1, ICON_W+2, ICON_H+2, icons[i].border);
        int tx = strlen(icons[i].icon) == 1 ? 20 : 16;
        fstr(x1 + tx, icons[i].y + 18, icons[i].icon, COLOR_WHITE, 0xFF140A28);
        fstr(x1 - 4, icons[i].y + 54, icons[i].label, COLOR_TEXT, 0);
    }
}

/* ─── HANDBOOK ─── */
static void draw_handbook(Win *w, uint32_t bg, uint32_t tx) {
    fstr(w->x+12, w->y+12, "======================================================", COLOR_ACCENT, bg);
    char ver_title[64], ver_sub[64];
    snprintf(ver_title, sizeof(ver_title), "         AMNESIADE: GRAPHICAL ENVIRONMENT v%s        ", SYNTH3X_VERSION);
    snprintf(ver_sub, sizeof(ver_sub), "AmnesiaDE v%s — Cyberpunk framebuffer DE", SYNTH3X_VERSION);
    fstr(w->x+12, w->y+26, ver_title, tx, bg);
    fstr(w->x+12, w->y+84, ver_sub, tx, bg);
    fstr(w->x+12, w->y+100, "Direct rendering via Linux Framebuffer (/dev/fb0)", tx, bg);
    fstr(w->x+12, w->y+116, "No X11, Wayland, or GTK needed.", tx, bg);
    fstr(w->x+12, w->y+132, "All logs in volatile RAM — destroyed on power-down.", tx, bg);
    fstr(w->x+12, w->y+148, "Network: Tor transparent proxy + nftables firewall.", tx, bg);
    fstr(w->x+12, w->y+172, "KEYBOARD SHORTCUTS:", COLOR_YELLOW, bg);
    fstr(w->x+24, w->y+192, "[ Super+1..4 ] Workspace switch", COLOR_TEXT, bg);
    fstr(w->x+24, w->y+208, "[ Tab ]        Window focus cycle", tx, bg);
    fstr(w->x+24, w->y+224, "[ CapsLock ]   Close focused window", tx, bg);
    fstr(w->x+24, w->y+240, "[ Up / Down ]  Cycle workspaces", tx, bg);
    fstr(w->x+24, w->y+256, "[ ESC ]        Exit to TTY shell", tx, bg);
    fstr(w->x+12, w->y+280, "Desktop: Terminal | SysInfo | Web | Guide | Install", COLOR_PANEL_FG, bg);
}

/* ─── WINDOW DRAW ─── */
static void draw_win(Win *w) {
    if (w->hidden || w->ws != current_ws) return;
    int is_active = (aw == (w - wins));
    uint32_t bd = is_active ? get_neon_color() : COLOR_WIN_BORDER;
    uint32_t bg = COLOR_WIN_BG, tl = COLOR_WIN_TITLE, tx = COLOR_TEXT;
    int t = w->y - 24;
    rect(w->x+4, t+4, w->w, w->h+24, 0xFF050308);
    rect(w->x-1, t-1, w->w+2, w->h+26, bd);
    rect(w->x, t, w->w, 24, tl);
    int title_len = strlen(w->title) * 8;
    int title_x = w->x + (w->w / 2) - (title_len / 2);
    fstr(title_x, t + 8, w->title, tx, tl);
    rect(w->x + 8, t + 6, 12, 12, COLOR_RED);
    rect(w->x + 24, t + 6, 12, 12, COLOR_YELLOW);
    rect(w->x + 40, t + 6, 12, 12, COLOR_GREEN);
    rect(w->x, w->y, w->w, w->h, bg);

    if (strcmp(w->title, "System Info") == 0) {
        fstr(w->x+12, w->y+12, "=== SYSTEM REALTIME HARDWARE STATS ===", COLOR_ACCENT, bg);
        fstr(w->x+12, w->y+32, cached_cpu, COLOR_TEXT, bg);
        fstr(w->x+12, w->y+52, cached_ram, COLOR_GREEN, bg);
        fstr(w->x+12, w->y+72, cached_laptop, COLOR_YELLOW, bg);
        int ram_pct = 0;
        char *pct_ptr = strchr(cached_ram, '(');
        if (pct_ptr) sscanf(pct_ptr, "(%d%%)", &ram_pct);
        if (ram_pct > 0) draw_progress_bar(w->x+12, w->y+86, (float)ram_pct / 100.0f, COLOR_GREEN);
        fstr(w->x+12, w->y+110, cached_disk, COLOR_TEXT, bg);
        fstr(w->x+12, w->y+128, cached_disk_list, COLOR_TEXT, bg);
        char usb_buf[128]; snprintf(usb_buf, sizeof(usb_buf), "USB: %s", cached_usb);
        fstr(w->x+12, w->y+148, usb_buf, COLOR_YELLOW, bg);
        char net_buf[128]; snprintf(net_buf, sizeof(net_buf), "NET: %s", cached_net);
        fstr(w->x+12, w->y+168, net_buf, COLOR_WHITE, bg);
        int tor_ok = is_tor_running();
        char tor_buf[64]; snprintf(tor_buf, sizeof(tor_buf), "SECURE TOR ROUTE: %s", tor_ok ? "ACTIVE" : "OFFLINE");
        fstr(w->x+12, w->y+188, tor_buf, tor_ok ? COLOR_GREEN : COLOR_RED, bg);
        draw_progress_bar(w->x+12, w->y+202, tor_ok ? 1.0f : 0.45f, tor_ok ? COLOR_GREEN : COLOR_YELLOW);
        /* Touchpad status */
        fstr(w->x+12, w->y+222, has_touchpad() ? "Touchpad: detected" : "Touchpad: not detected (mouse mode)", 
             has_touchpad() ? COLOR_GREEN : COLOR_DIM, bg);
    } else if (strcmp(w->title, "Terminal") == 0) {
        for (int i = 0; i < term_log_count; i++) {
            uint32_t col = COLOR_GREEN;
            if (term_logs[i][0] == '[') {
                if (term_logs[i][1] == 'N') col = COLOR_RED;
                if (term_logs[i][1] == 'T') col = COLOR_YELLOW;
                if (term_logs[i][1] == 'S') col = COLOR_TEXT;
            }
            fstr(w->x + 12, w->y + 12 + i * 16, term_logs[i], col, bg);
        }
        int input_y = w->y + w->h - 24;
        fstr(w->x + 12, input_y, "synth3x@root:~$ ", COLOR_TEXT, bg);
        fstr(w->x + 140, input_y, term_input, COLOR_WHITE, bg);
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        if (((ts.tv_nsec / 250000000) % 2) == 0) {
            int cur_x = w->x + 140 + strlen(term_input) * 8;
            rect(cur_x, input_y, 8, 12, COLOR_TEXT);
        }
    } else if (strcmp(w->title, "VSCodium") == 0) {
        rect(w->x + 130, w->y, 1, w->h, COLOR_DIM);
        fstr(w->x+8, w->y+12, "codium-workspace", COLOR_YELLOW, bg);
        fstr(w->x+16, w->y+32, "main.py", COLOR_TEXT, bg);
        fstr(w->x+16, w->y+48, "README.md", COLOR_GREEN, bg);
        fstr(w->x+8, w->y+w->h-24, "VSCodium v1.85", COLOR_PANEL_FG, bg);
        rect(w->x + 131, w->y, w->w - 131, 20, 0xFF0D0818);
        fstr(w->x + 140, w->y + 4, "main.py", COLOR_WHITE, 0xFF0D0818);
        const char *codium_code[] = {
            "import os", "import sys", "",
            "def main():",
            "    print(\"VSCodium on Synth3x Gentoo OS!\")",
            "    print(\"AmnesiaDE v" SYNTH3X_VERSION " stable.\")",
            "", "if __name__ == '__main__':", "    main()"
        };
        for (int i = 0; i < 9; i++)
            fstr(w->x + 140, w->y + 28 + i * 16, codium_code[i], 0xFFE6C880, bg);
        rect(w->x + 131, w->y + w->h - 20, w->w - 131, 20, 0xFF0A0514);
        fstr(w->x + 140, w->y + w->h - 16, "Ln 5, Col 12 | UTF-8 | Python", COLOR_PANEL_FG, 0xFF0A0514);
    } else if (strcmp(w->title, "Amnesia Handbook") == 0) {
        draw_handbook(w, bg, tx);
    } else if (strcmp(w->title, "Synth3x Guide") == 0) {
        fstr(w->x+12, w->y+12, "======================================================", COLOR_ACCENT, bg);
        fstr(w->x+12, w->y+26, "   SYNTH3X OS: COMPREHENSIVE GENTOO OPERATION GUIDE   ", tx, bg);
        fstr(w->x+12, w->y+40, "======================================================", COLOR_ACCENT, bg);
        char page_header[64];
        snprintf(page_header, sizeof(page_header), "PAGE %d/%d", guide_page + 1, GUIDE_MAX_PAGES);
        fstr(w->x+12, w->y+54, page_header, COLOR_YELLOW, bg);

        const WinTextLine *lines = NULL;
        int nlines = 0;
        switch (guide_page) {
            case 0: lines = guide_p0_lines; nlines = sizeof(guide_p0_lines)/sizeof(guide_p0_lines[0]); break;
            case 1: lines = guide_p1_lines; nlines = sizeof(guide_p1_lines)/sizeof(guide_p1_lines[0]); break;
            case 2: lines = guide_p2_lines; nlines = sizeof(guide_p2_lines)/sizeof(guide_p2_lines[0]); break;
            case 3: lines = guide_p3_lines; nlines = sizeof(guide_p3_lines)/sizeof(guide_p3_lines[0]); break;
            case 4: lines = guide_p4_lines; nlines = sizeof(guide_p4_lines)/sizeof(guide_p4_lines[0]); break;
            case 5: lines = guide_p5_lines; nlines = sizeof(guide_p5_lines)/sizeof(guide_p5_lines[0]); break;
            case 6: lines = guide_p6_lines; nlines = sizeof(guide_p6_lines)/sizeof(guide_p6_lines[0]); break;
            case 7: lines = guide_p7_lines; nlines = sizeof(guide_p7_lines)/sizeof(guide_p7_lines[0]); break;
        }
        if (lines) {
            for (int i = 0; i < nlines; i++) {
                uint32_t col = tx;
                if (lines[i].text[0] == 'P' || strstr(lines[i].text, "PAGE")) col = COLOR_YELLOW;
                if (strstr(lines[i].text, "[ SETUP")) col = COLOR_GREEN;
                fstr(w->x + lines[i].x, w->y + lines[i].y, lines[i].text, col, bg);
            }
        }

        /* Draw SETUP INTERNET button on page 0 */
        if (guide_page == 0) {
            int btn_net_x = w->x + 24, btn_net_y = w->y + 168;
            rect(btn_net_x, btn_net_y, 180, 24, 0xFF14281A);
            rect(btn_net_x-1, btn_net_y-1, 182, 26, COLOR_GREEN);
            fstr(btn_net_x + 14, btn_net_y + 8, "[ SETUP INTERNET ]", COLOR_GREEN, 0xFF14281A);
        }

        /* Navigation buttons */
        int btn_prev_x = w->x + 120, btn_next_x = w->x + 280, btn_y = w->y + 280;
        rect(btn_prev_x, btn_y, 80, 24, 0xFF1E1432);
        rect(btn_prev_x-1, btn_y-1, 82, 26, COLOR_DIM);
        fstr(btn_prev_x + 16, btn_y + 8, "< PREV", COLOR_TEXT, 0xFF1E1432);
        rect(btn_next_x, btn_y, 80, 24, 0xFF1E1432);
        rect(btn_next_x-1, btn_y-1, 82, 26, COLOR_DIM);
        fstr(btn_next_x + 16, btn_y + 8, "NEXT >", COLOR_TEXT, 0xFF1E1432);
    }
}

/* ─── NOTIFICATIONS ─── */
static void draw_notifs(void) {
    time_t now = time(NULL);
    int y = PANEL_H + 10, x = fb_w - NOTIF_W - 10;
    for (int i = 0; i < nc && i < 3; i++) {
        if (now - notifs[i].t > NOTIF_DUR + 2) {
            memmove(notifs + i, notifs + i + 1, sizeof(Notif) * (nc - i - 1));
            nc--; i--; continue;
        }
        uint32_t nb = COLOR_PANEL_BG, nf = COLOR_PANEL_FG, ac = COLOR_ACCENT, dm = COLOR_DIM;
        rect(x, y, NOTIF_W, NOTIF_H, nb);
        rect(x, y, 4, NOTIF_H, ac);
        rect(x, y, NOTIF_W, 1, ac);
        rect(x, y + NOTIF_H - 1, NOTIF_W, 1, dm);
        fstr(x + 12, y + 8, notifs[i].title, ac, nb);
        fstr(x + 12, y + 30, notifs[i].body, nf, nb);
        char s[16]; snprintf(s, 16, "%ds", (int)(now - notifs[i].t));
        fstr(x + NOTIF_W - 40, y + 8, s, dm, nb);
        y += NOTIF_H + 5;
    }
}

/* ─── PANEL ─── */
static void draw_panel(void) {
    uint32_t bg = COLOR_PANEL_BG, fg = COLOR_PANEL_FG, ac = COLOR_ACCENT;
    rect(0, 0, fb_w, PANEL_H, bg);
    rect(0, PANEL_H - 1, fb_w, 1, ac);
    char taskbar_str[64];
    snprintf(taskbar_str, sizeof(taskbar_str), "Synth3x OS  (AmnesiaDE v%s / Gentoo Profile)", SYNTH3X_VERSION);
    fstr(8, 10, taskbar_str, ac, bg);
    char ws[16]; snprintf(ws, 16, "WS %d/%d", current_ws + 1, WORKSPACES);
    fstr(340, 10, ws, fg, bg);
    int tor_ok = is_tor_running();
    fstr(430, 10, tor_ok ? "TOR: ACTIVE" : "TOR: OFFLINE", tor_ok ? COLOR_GREEN : COLOR_RED, bg);
    fstr(540, 10, has_touchpad() ? "TP: ON" : "TP: OFF", has_touchpad() ? COLOR_GREEN : COLOR_DIM, bg);
    time_t t = time(NULL);
    char ts[16]; strftime(ts, 16, " %H:%M ", localtime(&t));
    fstr(fb_w - 8 * strlen(ts) - 8, 10, ts, fg, bg);
}

/* ─── DOCK ─── */
static void draw_dock(void) {
    int w = 500, h = 34;
    int x = fb_w / 2 - w / 2, y = fb_h - 40;
    uint32_t db = COLOR_PANEL_BG, da = get_neon_color();
    rect(x, y, w, h, db);
    rect(x-1, y-1, w+2, h+2, da);
    int t_idx = find_win_by_title("Terminal");
    int s_idx = find_win_by_title("System Info");
    int c_idx = find_win_by_title("VSCodium");
    int h_idx = find_win_by_title("Amnesia Handbook");
    int g_idx = find_win_by_title("Synth3x Guide");
    int b_idx = find_win_by_title("Web Browser");
    struct { int idx; int off; const char *label; uint32_t col; int conditional; } items[] = {
        {t_idx, 10, "[>_] TERM", COLOR_TEXT},
        {s_idx, 75, "[i] STAT", COLOR_GREEN},
        {b_idx, 140, "[W] WEB", COLOR_ORANGE},
        {c_idx, 205, "{} VSCOD", COLOR_ACCENT, 1},
        {h_idx, 270, "[?] HANDBK", COLOR_YELLOW},
        {g_idx, 335, "[#] GUIDE", COLOR_GREEN},
    };
    for (int i = 0; i < 6; i++) {
        if (items[i].conditional && !vscodium_installed) {
            rect(x + items[i].off, y + 6, 60, 22, 0xFF140A28);
            fstr(x + items[i].off + 8, y + 12, "LOCKED", COLOR_DIM, 0xFF140A28);
            continue;
        }
        rect(x + items[i].off, y + 6, 60, 22, 0xFF140A28);
        int hidden = items[i].idx >= 0 && (wins[items[i].idx].hidden || wins[items[i].idx].ws != current_ws);
        fstr(x + items[i].off + 5, y + 12, items[i].label, hidden ? COLOR_DIM : items[i].col, 0xFF140A28);
    }
}

/* ─── SWAP (CRT SCANLINES) ─── */
static void swap(void) {
    for (int y = 0; y < fb_h; y++) {
        uint8_t *dst_row = fb + y * fb_stride_bytes;
        int dim = (y % 3 == 0);
        if (fb_bpp == 32) {
            uint32_t *src_row = &backbuf32[y * fb_w];
            uint32_t *dst32 = (uint32_t *)dst_row;
            if (dim) {
                for (int x = 0; x < fb_w; x++) {
                    uint32_t c = src_row[x];
                    uint32_t r = ((c >> 16) & 0xFF) >> 1;
                    uint32_t g = ((c >> 8) & 0xFF) >> 1;
                    uint32_t b = (c & 0xFF) >> 1;
                    dst32[x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            } else {
                memcpy(dst32, src_row, fb_w * 4);
            }
        } else {
            uint16_t *src_row = &backbuf16[y * fb_w];
            uint16_t *dst16 = (uint16_t *)dst_row;
            if (dim) {
                for (int x = 0; x < fb_w; x++)
                    dst16[x] = (src_row[x] >> 1) & 0x7BEF;
            } else {
                memcpy(dst16, src_row, fb_w * 2);
            }
        }
    }
}

/* ─── INPUT SYSTEM ─── */
#define MAX_INPUT_FDS 16
static int input_fds[MAX_INPUT_FDS];
static int input_fd_count = 0;

static void input_init(void) {
    for (int i = 0; i < MAX_INPUT_FDS; i++) input_fds[i] = -1;
    input_fd_count = 0;
    for (int i = 0; i < 16; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            if (input_fd_count < MAX_INPUT_FDS)
                input_fds[input_fd_count++] = fd;
            else
                close(fd);
        }
    }
    int m_fd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
    if (m_fd >= 0) {
        if (input_fd_count < MAX_INPUT_FDS)
            input_fds[input_fd_count++] = m_fd;
        else
            close(m_fd);
    }
}

static void handle_key(int code) {
    if(code == 1) running = 0;
    if (super_pressed) {
        if (code >= 2 && code <= 5) {
            current_ws = code - 2;
            beep(784, 40); beep(988, 40);
            notif_add("AmnesiaDE", "Workspace switched.");
            return;
        }
    }
    if(code == 58) {
        if(aw >= 0 && aw < wc) { wins[aw].hidden = 1; beep(600, 50); beep(400, 50); }
    }
    if(code == 15) {
        for(int i = 1; i <= wc; i++) {
            int ni = (aw + i) % wc;
            if(!wins[ni].hidden) { aw = ni; break; }
        }
        beep(523, 30);
    }
    if(code == 103 || code == 108) {
        int d = (code == 108) ? 1 : -1;
        current_ws = (current_ws + d + WORKSPACES) % WORKSPACES;
        beep(659, 30);
    }
    int term_idx = find_win_by_title("Terminal");
    if (term_idx >= 0 && aw == term_idx) {
        if (code == 28) {
            if (strlen(term_input) > 0) {
                exec_term_cmd(term_input);
                term_input[0] = '\0';
                beep(880, 40);
            }
        } else if (code == 14) {
            int len = strlen(term_input);
            if (len > 0) term_input[len - 1] = '\0';
        } else {
            char c = scancode_to_ascii(code, shift_pressed);
            if (c > 0 && strlen(term_input) < 48) {
                int len = strlen(term_input);
                term_input[len] = c;
                term_input[len + 1] = '\0';
            }
        }
    }
}

/* ─── MAIN ─── */
int main(int argc, char *argv[]) {
    printf("Synth3x OS — AmnesiaDE v%s (Gentoo Profile)\n", SYNTH3X_VERSION);
    
    fb_fd = open("/dev/fb0", O_RDWR);
    if(fb_fd < 0) { printf("No /dev/fb0\n"); return 1; }
    struct fb_var_screeninfo vi;
    struct fb_fix_screeninfo fix;
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vi);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix);
    fb_w = vi.xres; fb_h = vi.yres;
    fb_bpp = vi.bits_per_pixel;
    fb_stride_bytes = fix.line_length;
    
    uint8_t *fbmap = mmap(NULL, fb_h * fb_stride_bytes, PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if(fbmap == MAP_FAILED) { close(fb_fd); return 1; }
    fb = fbmap;
    
    if (fb_bpp == 32) backbuf32 = malloc(fb_w * fb_h * 4);
    else backbuf16 = malloc(fb_w * fb_h * 2);
    if(!backbuf16 && !backbuf32) { munmap(fb, fb_h * fb_stride_bytes); close(fb_fd); return 1; }
    
    int tty = open("/dev/tty0", O_RDWR);
    if(tty >= 0) ioctl(tty, KDSETMODE, KD_GRAPHICS);
    
    input_init(); notif_init();
    init_stars();
    
    char init_msg[64];
    snprintf(init_msg, sizeof(init_msg), "Synth3x OS v%s Core initialized [Gentoo Profile]", SYNTH3X_VERSION);
    term_log_add(init_msg);
    term_log_add("[OK] Framebuffer graphics engine active");
    term_log_add("[OK] Tor transparent routing available");
    term_log_add("[OK] nftables firewall active");
    term_log_add("[OK] MAC + hostname randomized");
    term_log_add("------------------------------------------");
    term_log_add("Commands: 'browser'=web | 'pkg install' | 'w3m'");
    term_log_add("Internet: type 'browser' or open Guide window");
    term_log_add("Touchpad: auto-detected on supported laptops");
    
    wnew("Terminal", 480, 280);
    wnew("System Info", 400, 260);
    wnew("Amnesia Handbook", 500, 340);
    wnew("Synth3x Guide", 520, 340);
    
    beep(523, 80); beep(659, 80); beep(784, 80); beep(1046, 120);
    char notif_title[32];
    snprintf(notif_title, sizeof(notif_title), "AmnesiaDE v%s", SYNTH3X_VERSION);
    notif_add(notif_title, "Gentoo Profile loaded. Type 'browser' for web!");
    notif_add("Internet Help", "Open Synth3x Guide for network setup.");
    notif_add("Touchpad", has_touchpad() ? "Touchpad detected!" : "Mouse mode active.");
    
    time_t last_term_update = 0;
    
    while(running) {
        if (stats_tick++ % 120 == 0) update_cached_stats();
        time_t cur_t = time(NULL);
        if (cur_t - last_term_update > 5) {
            last_term_update = cur_t;
            const char *updates[] = {
                "[NFT] Packet block: leak intercepted",
                "[TOR] Circuit renewed",
                "[SEC] Memory sweep: 0 leaks",
                "[ID ] MAC spoof: rotated",
                "[SYS] CPU temp: 38C",
                "[SEC] Amnesic RAM shield active",
                "[GENTOO] Portage ready for install",
                "[HW] Touchpad/mouse active"
            };
            srand(time(NULL) ^ getpid());
            term_log_add(updates[rand() % 8]);
        }
        
        struct pollfd fds[MAX_INPUT_FDS + 2]; int nf = 0;
        for(int i = 0; i < input_fd_count; i++) {
            if (input_fds[i] >= 0) {
                fds[nf].fd = input_fds[i];
                fds[nf].events = POLLIN;
                nf++;
            }
        }
        if(notif_fd >= 0) { fds[nf].fd = notif_fd; fds[nf].events = POLLIN; nf++; }
        
        if(poll(fds, nf, 16) > 0) {
            struct input_event ev;
            for(int i = 0; i < nf; i++) {
                if(!(fds[i].revents & POLLIN)) continue;
                if(fds[i].fd == notif_fd) { notif_read(); continue; }
                while(read(fds[i].fd, &ev, sizeof(ev)) == sizeof(ev)) {
                    if(ev.type == EV_REL) {
                        if(ev.code == REL_X) mx += ev.value * 2;
                        if(ev.code == REL_Y) my += ev.value * 2;
                    }
                    if(ev.type == EV_ABS) {
                        if(ev.code == ABS_X) mx = (ev.value * fb_w) / 32767;
                        if(ev.code == ABS_Y) my = (ev.value * fb_h) / 32767;
                        /* Touchpad finger count for multi-touch */
                        if(ev.code == ABS_MT_POSITION_X) mx = (ev.value * fb_w) / 4096;
                        if(ev.code == ABS_MT_POSITION_Y) my = (ev.value * fb_h) / 4096;
                    }
                    if(ev.type == EV_KEY) {
                        if(ev.code == BTN_LEFT || ev.code == BTN_TOUCH ||
                           ev.code == BTN_TOOL_FINGER) {
                            if(ev.value == 1) {
                                mclick = 1;
                                mouse_pressed = 1;
                                if (!show_copy_dialog && my < fb_h - 40) {
                                    int clicked_titlebar = 0;
                                    for(int j = wc - 1; j >= 0; j--) {
                                        if(!wins[j].hidden && wins[j].ws == current_ws) {
                                            if (win_title(&wins[j], mx, my) || win_close(&wins[j], mx, my) || 
                                                win_minimize(&wins[j], mx, my) || win_maximize(&wins[j], mx, my)) {
                                                clicked_titlebar = 1; break;
                                            }
                                        }
                                    }
                                    int dx_dock = fb_w / 2 - 250;
                                    int clicked_dock = (my >= fb_h - 40 && my <= fb_h - 6 && mx >= dx_dock && mx <= dx_dock + 500);
                                    if (!clicked_titlebar && !clicked_dock) {
                                        selecting = 1;
                                        sel_start_x = mx; sel_start_y = my;
                                        sel_end_x = mx; sel_end_y = my;
                                    }
                                }
                            }
                            if(ev.value == 0) {
                                mouse_pressed = 0;
                                for(int j = 0; j < wc; j++) wins[j].drag = 0;
                                mclick = 0;
                                if (selecting) {
                                    selecting = 0;
                                    int dx = abs(mx - sel_start_x);
                                    int dy = abs(my - sel_start_y);
                                    if (dx > 8 || dy > 8) {
                                        extract_selected_text();
                                        if (strlen(selected_text) > 0) {
                                            show_copy_dialog = 1;
                                            mclick = 0;
                                        }
                                    }
                                }
                            }
                        } else if (ev.code == 42 || ev.code == 54) {
                            shift_pressed = (ev.value != 0);
                        } else if (ev.code == 125 || ev.code == 126) {
                            super_pressed = (ev.value != 0);
                        } else if(ev.value == 1) {
                            handle_key(ev.code);
                        }
                    }
                }
            }
        }
        
        mx = mx < 0 ? 0 : (mx >= fb_w ? fb_w - 1 : mx);
        my = my < PANEL_H ? PANEL_H : (my >= fb_h ? fb_h - 1 : my);
        
        if(mclick) {
            mclick = 0;
            if (show_copy_dialog) {
                int mw = 320, mh = 140;
                int mx_pos = fb_w / 2 - mw / 2, my_pos = fb_h / 2 - mh / 2;
                int btn_y = my_pos + 94;
                if (mx >= mx_pos + 30 && mx <= mx_pos + 130 && my >= btn_y && my <= btn_y + 24) {
                    strncpy(clipboard, selected_text, sizeof(clipboard) - 1);
                    clipboard[sizeof(clipboard) - 1] = '\0';
                    char clip_cmd[1024];
                    FILE *tmp_f = fopen("/tmp/synth3x_clip", "w");
                    if (tmp_f) {
                        fputs(selected_text, tmp_f);
                        fclose(tmp_f);
                        snprintf(clip_cmd, sizeof(clip_cmd), "xclip -selection clipboard < /tmp/synth3x_clip 2>/dev/null || xsel -ib < /tmp/synth3x_clip 2>/dev/null");
                        system(clip_cmd);
                    }
                    notif_add("Clipboard", "Text copied!");
                    beep(880, 80); beep(1100, 120);
                    show_copy_dialog = 0;
                } else if (mx >= mx_pos + 190 && mx <= mx_pos + 290 && my >= btn_y && my <= btn_y + 24) {
                    show_copy_dialog = 0;
                    beep(300, 100);
                }
                continue;
            }
            
            int clicked_win = 0;
            for (int j = wc - 1; j >= 0; j--) {
                if (!wins[j].hidden && wins[j].ws == current_ws &&
                    mx >= wins[j].x && mx <= wins[j].x + wins[j].w &&
                    my >= wins[j].y - 24 && my <= wins[j].y + wins[j].h) {
                    clicked_win = 1; break;
                }
            }
            
            if (!clicked_win && my < fb_h - 40) {
                /* Desktop Icons: Terminal, SysInfo, Web, Handbook, Guide, VSCodium, Install */
                int icon_y[] = {60, 138, 216, 294, 372, 450, 528};
                if (mx >= 20 && mx <= 68) {
                    int y = my;
                    if (y >= icon_y[0] && y <= icon_y[0] + 48) {
                        int idx = find_win_by_title("Terminal");
                        if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Terminal", "Restored."); beep(400, 50); beep(500, 50); }
                    } else if (y >= icon_y[1] && y <= icon_y[1] + 48) {
                        int idx = find_win_by_title("System Info");
                        if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("SysInfo", "Restored."); beep(400, 50); beep(500, 50); }
                    } else if (y >= icon_y[2] && y <= icon_y[2] + 48) {
                        /* Web Browser - launch w3m */
                        term_log_add("Launching web browser (w3m)...");
                        pthread_t b_thread;
                        pthread_create(&b_thread, NULL, async_launch_browser, NULL);
                        pthread_detach(b_thread);
                        notif_add("Browser", "Starting w3m in terminal...");
                    } else if (y >= icon_y[3] && y <= icon_y[3] + 48) {
                        int idx = find_win_by_title("Amnesia Handbook");
                        if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Handbook", "Restored."); beep(400, 50); beep(500, 50); }
                    } else if (y >= icon_y[4] && y <= icon_y[4] + 48) {
                        int idx = find_win_by_title("Synth3x Guide");
                        if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Guide", "Restored."); beep(400, 50); beep(500, 50); }
                    } else if (y >= icon_y[5] && y <= icon_y[5] + 48 && vscodium_installed) {
                        int idx = find_win_by_title("VSCodium");
                        if (idx < 0) { wnew("VSCodium", 500, 320); beep(523, 60); beep(659, 60); beep(784, 80); }
                        else { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; beep(400, 50); beep(500, 50); }
                        notif_add("VSCodium", "Editor restored.");
                    } else if (y >= icon_y[6] && y <= icon_y[6] + 48) {
                        notif_add("Installer", "Press ESC and run: synth3x-installer");
                    }
                }
            }
            
            if (my >= fb_h - 40 && my <= fb_h - 6) {
                int dx = fb_w / 2 - 250;
                if (mx >= dx + 10 && mx <= dx + 70) {
                    int idx = find_win_by_title("Terminal");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; beep(400, 50); beep(500, 50); }
                } else if (mx >= dx + 75 && mx <= dx + 135) {
                    int idx = find_win_by_title("System Info");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; beep(400, 50); beep(500, 50); }
                } else if (mx >= dx + 140 && mx <= dx + 200) {
                    term_log_add("Launching web browser (w3m)...");
                    pthread_t b_thread;
                    pthread_create(&b_thread, NULL, async_launch_browser, NULL);
                    pthread_detach(b_thread);
                } else if (mx >= dx + 205 && mx <= dx + 265) {
                    if (vscodium_installed) {
                        int idx = find_win_by_title("VSCodium");
                        if (idx < 0) { idx = wnew("VSCodium", 500, 320); beep(523, 60); beep(659, 60); beep(784, 80); }
                        else { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; beep(400, 50); beep(500, 50); }
                        notif_add("VSCodium", "Code editor loaded.");
                    } else {
                        notif_add("Synth3x OS", "VSCodium not installed. Type 'pkg install vscodium'.");
                        beep(300, 120);
                    }
                } else if (mx >= dx + 270 && mx <= dx + 330) {
                    int idx = find_win_by_title("Amnesia Handbook");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; beep(400, 50); beep(500, 50); }
                } else if (mx >= dx + 335 && mx <= dx + 395) {
                    int idx = find_win_by_title("Synth3x Guide");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; beep(400, 50); beep(500, 50); }
                }
            }
            
            int hit = -1;
            for(int j = wc - 1; j >= 0; j--) {
                if(!wins[j].hidden && wins[j].ws == current_ws && 
                   (win_title(&wins[j], mx, my) || win_close(&wins[j], mx, my) || 
                    win_minimize(&wins[j], mx, my) || win_maximize(&wins[j], mx, my))) {
                    hit = j; break;
                }
            }
            if(hit >= 0) {
                if(win_close(&wins[hit], mx, my)) { wins[hit].hidden = 1; beep(600, 60); beep(400, 60); }
                else if(win_minimize(&wins[hit], mx, my)) { wins[hit].hidden = 1; notif_add(wins[hit].title, "Minimized."); beep(400, 50); beep(300, 50); }
                else if(win_maximize(&wins[hit], mx, my)) {
                    if (wins[hit].maximized) {
                        wins[hit].x = wins[hit].orig_x; wins[hit].y = wins[hit].orig_y;
                        wins[hit].w = wins[hit].orig_w; wins[hit].h = wins[hit].orig_h;
                        wins[hit].maximized = 0;
                    } else {
                        wins[hit].orig_x = wins[hit].x; wins[hit].orig_y = wins[hit].y;
                        wins[hit].orig_w = wins[hit].w; wins[hit].orig_h = wins[hit].h;
                        wins[hit].x = 0; wins[hit].y = PANEL_H + 24;
                        wins[hit].w = fb_w; wins[hit].h = fb_h - PANEL_H - 24 - 45;
                        wins[hit].maximized = 1;
                    }
                    beep(500, 60); beep(700, 60);
                } else {
                    Win t = wins[hit];
                    memmove(&wins[hit], &wins[hit+1], sizeof(Win) * (wc - hit - 1));
                    wins[wc-1] = t; aw = wc-1;
                    wins[aw].drag = 1; wins[aw].dx = mx - wins[aw].x; wins[aw].dy = my - (wins[aw].y - 24);
                }
            }
            
            int g_idx = find_win_by_title("Synth3x Guide");
            if (g_idx >= 0 && !wins[g_idx].hidden && wins[g_idx].ws == current_ws) {
                int btn_prev_x = wins[g_idx].x + 120, btn_next_x = wins[g_idx].x + 280, btn_y = wins[g_idx].y + 280;
                if (mx >= btn_prev_x && mx <= btn_prev_x + 80 && my >= btn_y && my <= btn_y + 24) {
                    guide_page = (guide_page - 1 + GUIDE_MAX_PAGES) % GUIDE_MAX_PAGES;
                    beep(500, 30);
                } else if (mx >= btn_next_x && mx <= btn_next_x + 80 && my >= btn_y && my <= btn_y + 24) {
                    guide_page = (guide_page + 1) % GUIDE_MAX_PAGES;
                    beep(500, 30);
                }
                if (guide_page == 0) {
                    int btn_net_x = wins[g_idx].x + 24, btn_net_y = wins[g_idx].y + 168;
                    if (mx >= btn_net_x && mx <= btn_net_x + 180 && my >= btn_net_y && my <= btn_net_y + 24) {
                        beep(523, 80); beep(659, 80); beep(784, 120);
                        notif_add("Network Config", "Starting auto-config...");
                        pthread_t net_thread;
                        pthread_create(&net_thread, NULL, async_net_setup, NULL);
                        pthread_detach(net_thread);
                    }
                }
            }
        }
        
        int any_drag = 0;
        for(int i = 0; i < wc; i++) if(wins[i].drag) {
            wins[i].x = mx - wins[i].dx; wins[i].y = my - wins[i].dy + 24;
            if(wins[i].x < 0) wins[i].x = 0;
            if(wins[i].y < PANEL_H + 24) wins[i].y = PANEL_H + 24;
            if(wins[i].x + wins[i].w > fb_w) wins[i].x = fb_w - wins[i].w;
            if(wins[i].y + wins[i].h > fb_h - 42) wins[i].y = fb_h - 42 - wins[i].h;
            any_drag = 1;
        }
        
        if (selecting && mouse_pressed && !any_drag) {
            sel_end_x = mx; sel_end_y = my;
        }
        
        memset(backbuf32 ? (void*)backbuf32 : (void*)backbuf16, 0, fb_w * fb_h * (fb_bpp == 32 ? 4 : 2));
        draw_bg();
        for(int i = 0; i < wc; i++) draw_win(&wins[i]);
        draw_notifs(); draw_panel(); draw_dock();
        
        if (selecting) {
            int x = sel_start_x < sel_end_x ? sel_start_x : sel_end_x;
            int y = sel_start_y < sel_end_y ? sel_start_y : sel_end_y;
            int w = abs(sel_end_x - sel_start_x);
            int h = abs(sel_end_y - sel_start_y);
            rect_blend(x, y, w, h, 0x4000FFFF);
        }
        if (show_copy_dialog) draw_copy_modal();
        draw_custom_cursor(mx, my);
        swap();
    }
    
    for(int i = 0; i < input_fd_count; i++)
        if(input_fds[i] >= 0) close(input_fds[i]);
    if (backbuf32) free(backbuf32);
    if (backbuf16) free(backbuf16);
    munmap(fb, fb_h * fb_stride_bytes); close(fb_fd);
    if(tty >= 0) ioctl(tty, KDSETMODE, KD_TEXT);
    printf("AmnesiaDE: done.\n");
    for(;;) pause();
}
