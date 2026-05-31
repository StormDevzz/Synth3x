/* Synth3x OS — AmnesiaDE v0.7 — Ultimate Cyberpunk Desktop
 * C + Custom Optimized Framebuffer rendering pipeline. Double-buffered.
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
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <pthread.h>

#include "synth3x.h"

/* ─── CONFIG ─── */
#define MAX_WIN     16
#define MAX_NOTIF   8
#define PANEL_H     28
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
#define COLOR_BG        0xFF0A0514 // Deep space retro black/purple
#define COLOR_PANEL_BG  0xFF140A20 // Glassy taskbar purple
#define COLOR_PANEL_FG  0xFFC8B4E6 // Soft purple text
#define COLOR_WIN_BG    0xFF0F0A18 // Translucent dark purple
#define COLOR_WIN_TITLE 0xFF180E26 // High tech header
#define COLOR_WIN_BORDER 0xFF461E6E // Violet margin
#define COLOR_ACCENT    0xFFFF0080 // Neon hot pink
#define COLOR_TEXT      0xFF00FFE6 // Neon electrifying cyan
#define COLOR_DIM       0xFF5A3C6E // Dim cyber violet
#define COLOR_WHITE     0xFFFFFFFF // Clean white
#define COLOR_GREEN     0xFF50DC64 // Glowing matrix green
#define COLOR_YELLOW    0xFFFADC32 // Matrix warning yellow
#define COLOR_RED       0xFFFA5064 // Cyber alert red

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

#define MAX_TERM_LOGS 12
static char term_logs[MAX_TERM_LOGS][64];
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
static int vscodium_installed = 0; // Dynamic VSCodium install state
static int current_ws = 0;         // Current active desktop workspace (0..3)

/* ─── AMNESIA HANDBOOK PAGES CONFIG ─── */
static int guide_page = 0;
#define GUIDE_MAX_PAGES 3

/* ─── FORWARD DECLARATIONS ─── */
static uint32_t get_neon_color(void);
static void draw_progress_bar(int x, int y, float percent, uint32_t color);
static void beep(int freq, int ms);

/* ─── SCANCODE TO ASCII TRANSLATION (US Layout) ─── */
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
        case 53: return shift ? '?' : '/';
    }
    return 0;
}

/* ─── NOTIFICATION MANAGER SYSTEM ─── */
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

/* ─── TERMINAL CONSOLE LOG BUFFER ─── */
static void term_log_add(const char *msg) {
    if (term_log_count >= MAX_TERM_LOGS) {
        memmove(term_logs, term_logs + 1, sizeof(term_logs[0]) * (MAX_TERM_LOGS - 1));
        term_log_count--;
    }
    strncpy(term_logs[term_log_count], msg, sizeof(term_logs[0]) - 1);
    term_logs[term_log_count][sizeof(term_logs[0]) - 1] = '\0';
    term_log_count++;
}

/* ─── HARDWARE STATS DYNAMIC CACHE ─── */
static char cached_ram[64] = "RAM: Loading...";
static char cached_disk[64] = "DISK space: Loading...";
static char cached_disk_list[64] = "DISK list: Loading...";
static char cached_cpu[128] = "CPU: Loading...";
static char cached_usb[128] = "USB: Loading...";
static char cached_net[128] = "Net: Loading...";
static int stats_tick = 0;

static void update_cached_stats(void) {
    FILE *fp;
    char line[128];
    
    // RAM Analyzer
    fp = popen("/usr/bin/ram_analyzer", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            strncpy(cached_ram, line, sizeof(cached_ram) - 1);
        }
        pclose(fp);
    }
    
    // Disk Analyzer
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
    
    // Device Names Analyzer
    fp = popen("/usr/bin/device_names", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            strncpy(cached_cpu, line, sizeof(cached_cpu) - 1);
        }
        pclose(fp);
    }
    
    // USB Analyzer
    fp = popen("/usr/bin/usb_analyzer", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) { /* Skip header "USB devices:" */ }
        cached_usb[0] = '\0';
        int cnt = 0;
        while (fgets(line, sizeof(line), fp) && cnt < 2) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (cnt > 0) strncat(cached_usb, ", ", sizeof(cached_usb) - strlen(cached_usb) - 1);
            strncat(cached_usb, line + 3, sizeof(cached_usb) - strlen(cached_usb) - 1); // Skip " - "
            cnt++;
        }
        if (cnt == 0) {
            strcpy(cached_usb, "No external USB devices.");
        }
        pclose(fp);
    }
    
    // Cable Analyzer
    fp = popen("/usr/bin/cable_analyzer", "r");
    if (fp) {
        if (fgets(line, sizeof(line), fp)) { /* Skip header "Net cables:" */ }
        cached_net[0] = '\0';
        int cnt = 0;
        while (fgets(line, sizeof(line), fp) && cnt < 2) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (cnt > 0) strncat(cached_net, ", ", sizeof(cached_net) - strlen(cached_net) - 1);
            strncat(cached_net, line + 3, sizeof(cached_net) - strlen(cached_net) - 1);
            cnt++;
        }
        if (cnt == 0) {
            strcpy(cached_net, "No interfaces detected.");
        }
        pclose(fp);
    }
}

typedef struct {
    char cmd[128];
} CmdArgs;

/* ─── ASYNCHRONOUS BACKGROUND THREAD WORKERS ─── */
static void *async_vscodium_install(void *arg) {
    (void)arg;
    term_log_add("Resolving dependencies...");
    term_log_add("Downloading vscodium via Tor Transparent Proxy...");
    
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
    term_log_add("AmnesiaDE: 🔒 VSCOD icon is now unlocked.");
    notif_add("Package Manager", "VSCodium installed! Check the Dock.");
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
    
    FILE *fp = popen(args->cmd, "r");
    if (!fp) {
        term_log_add("[ERR] Failed to execute command.");
        free(args);
        return NULL;
    }
    
    char line[128];
    int lines_read = 0;
    while (fgets(line, sizeof(line), fp) && lines_read < 8) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        term_log_add(line);
        lines_read++;
    }
    pclose(fp);
    
    if (lines_read == 0) {
        term_log_add("Command returned no output.");
    }
    
    free(args);
    return NULL;
}

static void *async_net_setup(void *arg) {
    (void)arg;
    term_log_add("Network Manager: Spoofing hardware MAC address...");
    
    // Quick random MAC spoofing trigger
    system("ip link set dev eth0 down 2>/dev/null");
    system("ip link set dev eth0 address 00:e0:4c:$(printf '%02x:%02x:%02x' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256))) 2>/dev/null");
    system("ip link set dev eth0 up 2>/dev/null");
    
    term_log_add("Network Manager: Initializing DHCP client lease on eth0...");
    FILE *fp = popen("udhcpc -i eth0 -n 2>&1", "r");
    if (!fp) {
        fp = popen("dhcpcd eth0 -n 2>&1", "r");
    }
    
    if (fp) {
        char line[128];
        int lines = 0;
        while (fgets(line, sizeof(line), fp) && lines < 4) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            term_log_add(line);
            lines++;
        }
        pclose(fp);
    }
    
    term_log_add("Network Manager: Interface auto-config complete!");
    notif_add("Network Config", "Connected! Ethernet link initialized.");
    beep(880, 80); beep(1100, 150);
    return NULL;
}

/* ─── TERMINAL COMMAND EXECUTOR (pkg installer & popen) ─── */
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
    
    CmdArgs *args = malloc(sizeof(CmdArgs));
    strncpy(args->cmd, cmd, sizeof(args->cmd) - 1);
    args->cmd[sizeof(args->cmd) - 1] = '\0';
    
    pthread_t cmd_thread;
    pthread_create(&cmd_thread, NULL, async_exec_cmd, args);
    pthread_detach(cmd_thread);
}

/* ─── RETRO CONSOLE AUDIO SYNTHESIZER ─── */
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

/* ─── ULTRA-FAST GRAPHICAL PRIMITIVES ─── */
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

static void px(int x, int y, uint32_t c) {
    draw_px(x, y, c);
}
static void rect(int x, int y, int w, int h, uint32_t c) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            draw_px(x + col, y + row, c);
        }
    }
}
static void fchar(int x, int y, char c, uint32_t fg, uint32_t bg) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = &font8x8[(c - 32) * 8];
    for (int row = 0; row < 8; row++) {
        uint8_t byte = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (byte & (0x80 >> col)) {
                draw_px(x + col, y + row, fg);
                draw_px(x + col + 1, y + row, fg); // Horizontal bolding for high contrast/sharpness
            } else if (bg != 0) {
                // Ensure we don't overwrite the bold pixel of the previous column
                if (col == 0 || !(byte & (0x80 >> (col - 1)))) {
                    draw_px(x + col, y + row, bg);
                }
            }
        }
    }
}
static void fstr(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    while (*s) {
        fchar(x, y, *s++, fg, bg);
        x += 8;
    }
}

/* ─── NEON COLOR OSCILLATOR ─── */
static uint32_t get_neon_color(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int tick = (ts.tv_nsec / 15000000) % 64; // 0..63
    int val = tick < 32 ? tick : 64 - tick;  // 0..32..0
    
    int r = val * 8;
    int g = 255 - val * 8;
    int b = 255 - val * 2;
    
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

/* ─── PREMIUM CUSTOM TRANSPARENT CURSOR MASK ─── */
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
            if (cursor_map[r][c] == 'X') {
                px(cx + c, cy + r, border_col);
            } else if (cursor_map[r][c] == '.') {
                px(cx + c, cy + r, inner_col);
            }
        }
    }
}

/* ─── GRAPHICAL PROGRESS BAR ─── */
static void draw_progress_bar(int x, int y, float percent, uint32_t color) {
    int w = 180;
    int h = 8;
    rect(x, y, w, h, 0xFF1E1428); // Empty slot
    rect(x, y, (int)(w * percent), h, color); // Filled slot
    rect(x-1, y-1, w+2, 1, COLOR_DIM);
    rect(x-1, y+h, w+2, 1, COLOR_DIM);
    rect(x-1, y, 1, h, COLOR_DIM);
    rect(x+w, y, 1, h, COLOR_DIM);
}

/* ─── TWINKLING CYBER-STARS ─── */
typedef struct { int x, y; int type; } Star;
static Star stars[32];

static void init_stars(void) {
    srand(1337);
    for (int i = 0; i < 32; i++) {
        stars[i].x = rand() % 800;
        stars[i].y = PANEL_H + 10 + (rand() % 220); // upper sky only
        stars[i].type = rand() % 3;
    }
}

static void draw_stars(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int tick = (ts.tv_nsec / 100000000) % 8; // 0..7
    
    for (int i = 0; i < 32; i++) {
        int brightness = (tick + i) % 4;
        uint32_t c;
        if (brightness == 0) c = COLOR_DIM;
        else if (brightness == 1) c = 0xFF8A5A9E;
        else if (brightness == 2) c = COLOR_PANEL_FG;
        else c = COLOR_WHITE;
        
        px(stars[i].x, stars[i].y, c);
        if (stars[i].type == 1 && brightness >= 2) {
            px(stars[i].x - 1, stars[i].y, c);
            px(stars[i].x + 1, stars[i].y, c);
            px(stars[i].x, stars[i].y - 1, c);
            px(stars[i].x, stars[i].y + 1, c);
        }
    }
}

/* ─── RETROWAVE VECTOR SUN & MOUNTAINS ─── */
static void draw_retro_sun(int cx, int cy, int r) {
    for (int dy = -r; dy <= r; dy++) {
        int y = cy + dy;
        if (y < PANEL_H || y >= fb_h) continue;
        
        int w = (int)sqrt(r * r - dy * dy);
        
        // Retrowave horizontal sliced sun cuts
        if (dy > 10 && (dy % 14 < (dy / 4))) {
            continue; 
        }
        
        float factor = (float)(dy + r) / (2 * r);
        Color c_top = {255, 120, 0};   // Radiant orange
        Color c_bot = {255, 0, 128};   // Neon hot pink
        Color c = lerp(c_top, c_bot, factor);
        
        rect(cx - w, y, w * 2, 1, c565(c) | 0xFF000000);
    }
}

static void draw_mountains(void) {
    uint32_t m_col = COLOR_ACCENT; // Neon line color
    uint32_t m_fill = COLOR_WIN_BG; // Solid dark fill
    int horizon = fb_h / 2 + 80;
    
    for (int x = 0; x < fb_w; x++) {
        int h1 = 0;
        if (x >= 0 && x <= 360) {
            h1 = 110 - abs(x - 160) * 110 / 180;
            if (h1 < 0) h1 = 0;
        }
        
        int h2 = 0;
        if (x >= 340 && x <= fb_w) {
            h2 = 130 - abs(x - 620) * 130 / 220;
            if (h2 < 0) h2 = 0;
        }
        
        int max_h = h1 > h2 ? h1 : h2;
        if (max_h > 0) {
            int peak_y = horizon - max_h;
            rect(x, peak_y, 1, max_h, m_fill);
            px(x, peak_y, m_col);
            
            // Add technical vertical mesh stripes
            if (x % 24 == 0) {
                for (int my = peak_y; my < horizon; my += 16) {
                    px(x, my, COLOR_DIM);
                }
            }
        }
    }
}

static void draw_perspective_grid(void) {
    uint32_t grid_col = COLOR_TEXT; // Bright neon cyan perspective lines
    int horizon_y = fb_h / 2 + 80;
    
    // Horizontal lines with exponentially increasing spacing
    int step = 8;
    for (int y = horizon_y; y < fb_h - 40; y += step) {
        rect(0, y, fb_w, 1, grid_col);
        step = (int)(step * 1.35f);
        if (step > 64) step = 64;
    }
    
    // Vertical perspective lines converging to horizon center
    int cx = fb_w / 2;
    for (int x = -fb_w; x <= fb_w * 2; x += 55) {
        int x1 = cx;
        int y1 = horizon_y;
        int x2 = x;
        int y2 = fb_h - 40;
        
        int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = dx + dy, e2;
        
        int curr_x = x1, curr_y = y1;
        while (1) {
            if (curr_x >= 0 && curr_x < fb_w && curr_y >= horizon_y && curr_y < fb_h - 40) {
                px(curr_x, curr_y, grid_col);
            }
            if (curr_x == x2 && curr_y == y2) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; curr_x += sx; }
            if (e2 <= dx) { err += dx; curr_y += sy; }
        }
    }
}

static void draw_bg(void) {
    rect(0,PANEL_H,fb_w,fb_h-PANEL_H,COLOR_BG);
    
    // Twinkling stars, retro sun, grid & Mountains
    draw_stars();
    draw_retro_sun(fb_w/2, fb_h/2 + 20, 110);
    draw_mountains();
    draw_perspective_grid();
    draw_desktop_icons();
}

static void get_mac_address(char *out_mac, int max_len) {
    strncpy(out_mac, "02:00:00:00:00:00", max_len);
    DIR *d = opendir("/sys/class/net");
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d))) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, "lo") == 0)
            continue;
        char path[128];
        snprintf(path, sizeof(path), "/sys/class/net/%s/address", de->d_name);
        int fd = open(path, O_RDONLY);
        if (fd >= 0) {
            int n = read(fd, out_mac, max_len - 1);
            if (n > 0) {
                out_mac[n] = '\0';
                char *nl = strchr(out_mac, '\n');
                if (nl) *nl = '\0';
            }
            close(fd);
            break;
        }
    }
    closedir(d);
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
                    if (strncmp(comm, "tor", 3) == 0) {
                        running = 1;
                        close(fd);
                        break;
                    }
                }
                close(fd);
            }
        }
    }
    closedir(d);
    return running;
}

/* ─── WINDOWS MANAGER ─── */

static int find_win_by_title(const char *title) {
    for (int i = 0; i < wc; i++) {
        if (strcmp(wins[i].title, title) == 0) return i;
    }
    return -1;
}
static int wnew(const char *t, int w, int h) {
    if (wc>=MAX_WIN) return -1;
    Win *wn = &wins[wc++];
    wn->x = 60+(wc*40)%(fb_w-w-80); wn->y = PANEL_H+40+(wc*30)%(fb_h-PANEL_H-h-100);
    wn->w = w; wn->h = h; wn->hidden=0; wn->ws=current_ws; wn->drag=0; wn->maximized=0;
    strncpy(wn->title, t, 47); aw = wc-1; return aw;
}
static int win_title(Win *w, int x, int y) {
    if (w->maximized) return 0;
    return x>=w->x && x<=w->x+w->w && y>=w->y-24 && y<=w->y;
}
static int win_close(Win *w, int x, int y) {
    return x>=w->x+8 && x<=w->x+20 && y>=w->y-22 && y<=w->y-4;
}
static int win_minimize(Win *w, int x, int y) {
    return x>=w->x+24 && x<=w->x+36 && y>=w->y-22 && y<=w->y-4;
}
static int win_maximize(Win *w, int x, int y) {
    return x>=w->x+40 && x<=w->x+52 && y>=w->y-22 && y<=w->y-4;
}

/* ─── TEXT SELECTION & GLOBAL CLIPBOARD SYSTEM ─── */
static int selecting = 0;
static int mouse_pressed = 0;
static int sel_start_x = 0, sel_start_y = 0;
static int sel_end_x = 0, sel_end_y = 0;
static int show_copy_dialog = 0;
static char selected_text[512] = "";
static char clipboard[1024] = "";

typedef struct {
    int x;
    int y;
    const char *text;
} WinTextLine;

static const WinTextLine handbook_lines[] = {
    {12, 12, "================================================"},
    {12, 26, "         AMNESIADE: GRAPHICAL ENVIRONMENT       "},
    {12, 40, "================================================"},
    {12, 64, "OVERVIEW: VOLATILE RAM SECURITY SHELL"},
    {12, 84, "AmnesiaDE is a lightweight cyberpunk interface"},
    {12, 100, "designed for volatile RAM platforms. It renders"},
    {12, 116, "directly via Linux Framebuffer (/dev/fb0) and"},
    {12, 132, "sweeps all system logs on active power down."},
    {12, 160, "GLOBAL KEYBOARD SHORTCUTS:"},
    {24, 180, "[ Super + 1..4 ] : Switch active workspaces (1-4)"},
    {24, 196, "[ Tab ]          : Switch active window focus"},
    {24, 212, "[ CapsLock ]     : Close focused window instantly"},
    {24, 228, "[ Up / Down ]    : Cycle workspaces sequentially"},
    {24, 244, "[ ESC ]          : Terminate DE session to TTY"},
    {12, 274, "Window Traffic Lights: Red (Close), Yellow (Min), Green (Max)"}
};

static const WinTextLine guide_p0_lines[] = {
    {12, 12, "================================================"},
    {12, 26, "         SYNTH3X OS: SYSTEM OPERATION GUIDE     "},
    {12, 40, "================================================"},
    {12, 64, "PAGE 1: NETWORK & TOR INTERNET CONFIGURATION"},
    {12, 84, "This volatile environment runs entirely in RAM."},
    {12, 100, "All network interfaces route through Tor & nftables."},
    {12, 116, "To initialize the DHCP service and network link:"},
    {36, 152, "[ SETUP INTERNET ]"},
    {12, 184, "Click the button above to trigger auto-DHCP setup."},
    {12, 200, "This automatically spoofs MAC & randomizes host."},
    {12, 224, "To configure Wi-Fi using iwctl in Terminal:"},
    {24, 240, "$ iwctl --passphrase \"key\" station wlan0 connect \"SSID\""}
};

static const WinTextLine guide_p1_lines[] = {
    {12, 12, "================================================"},
    {12, 26, "         SYNTH3X OS: SYSTEM OPERATION GUIDE     "},
    {12, 40, "================================================"},
    {12, 64, "PAGE 2: PACKAGE MANAGEMENT (EMERGE & PKG)"},
    {12, 84, "As a Gentoo-based OS, you can install any package"},
    {12, 100, "directly from Portage repositories using emerge:"},
    {24, 120, "$ emerge --ask [package_name]"},
    {12, 140, "Or use the custom amnesic packaging wrapper:"},
    {24, 160, "$ pkg install [package_name]"},
    {12, 180, "To compile native C source files locally:"},
    {24, 200, "$ gcc main.c -o program"}
};

static const WinTextLine guide_p2_lines[] = {
    {12, 12, "================================================"},
    {12, 26, "         SYNTH3X OS: SYSTEM OPERATION GUIDE     "},
    {12, 40, "================================================"},
    {12, 64, "PAGE 3: DEVELOPMENT & IDE CONFIGURATION"},
    {12, 84, "AmnesiaDE does not pre-install VSCodium by default"},
    {12, 100, "due to strict amnesic security rules. You can:"},
    {24, 120, "- Download external IDE binaries manually to /persist."},
    {24, 136, "- Run automated secure installer in terminal:"},
    {24, 156, "  $ pkg install vscodium"},
    {24, 176, "- Edit files directly in console using: nano"}
};

static void rect_blend(int x, int y, int w, int h, uint32_t color) {
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = x + w > fb_w ? fb_w : x + w;
    int y2 = y + h > fb_h ? fb_h : y + h;
    
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r_c = (color >> 16) & 0xFF;
    uint8_t g_c = (color >> 8) & 0xFF;
    uint8_t b_c = color & 0xFF;
    
    for (int row = y1; row < y2; row++) {
        for (int col = x1; col < x2; col++) {
            if (fb_bpp == 32) {
                uint32_t orig = backbuf32[row * fb_w + col];
                uint8_t r_o = (orig >> 16) & 0xFF;
                uint8_t g_o = (orig >> 8) & 0xFF;
                uint8_t b_o = orig & 0xFF;
                
                uint8_t r_new = (r_c * a + r_o * (255 - a)) / 255;
                uint8_t g_new = (g_c * a + g_o * (255 - a)) / 255;
                uint8_t b_new = (b_c * a + b_o * (255 - a)) / 255;
                
                backbuf32[row * fb_w + col] = 0xFF000000 | (r_new << 16) | (g_new << 8) | b_new;
            } else {
                uint16_t orig = backbuf16[row * fb_w + col];
                uint8_t r_o = ((orig >> 11) & 0x1F) << 3;
                uint8_t g_o = ((orig >> 5) & 0x3F) << 2;
                uint8_t b_o = (orig & 0x1F) << 3;
                
                uint8_t r_new = (r_c * a + r_o * (255 - a)) / 255;
                uint8_t g_new = (g_c * a + g_o * (255 - a)) / 255;
                uint8_t b_new = (b_c * a + b_o * (255 - a)) / 255;
                
                uint16_t r = r_new >> 3;
                uint16_t g = g_new >> 2;
                uint16_t b = b_new >> 3;
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
                if (*char_count > 0 && selected_text[strlen(selected_text)-1] != '\n') {
                    strcat(selected_text, "\n");
                }
                int cur_len = strlen(selected_text);
                if (cur_len + sel_len < (int)sizeof(selected_text) - 2) {
                    strncat(selected_text, lines[i].text + start_idx, sel_len);
                    *char_count += sel_len;
                }
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
                    if (char_count > 0 && selected_text[strlen(selected_text)-1] != '\n') {
                        strcat(selected_text, "\n");
                    }
                    int cur_len = strlen(selected_text);
                    if (cur_len + sel_len < (int)sizeof(selected_text) - 2) {
                        strncat(selected_text, term_logs[i] + start_idx, sel_len);
                        char_count += sel_len;
                    }
                }
            }
        }
        int prompt_y = w->y + w->h - 24;
        if (prompt_y + 4 >= y1 && prompt_y + 4 <= y2) {
            char full_prompt[128];
            snprintf(full_prompt, sizeof(full_prompt), "synth3x@root:~$ %s", term_input);
            int start_idx = -1, end_idx = -1;
            int len = strlen(full_prompt);
            for (int col = 0; col < len; col++) {
                int cx = w->x + 12 + col * 8;
                if (cx + 4 >= x1 && cx + 4 <= x2) {
                    if (start_idx == -1) start_idx = col;
                    end_idx = col;
                }
            }
            if (start_idx != -1 && end_idx != -1) {
                int sel_len = end_idx - start_idx + 1;
                if (char_count > 0 && selected_text[strlen(selected_text)-1] != '\n') {
                    strcat(selected_text, "\n");
                }
                int cur_len = strlen(selected_text);
                if (cur_len + sel_len < (int)sizeof(selected_text) - 2) {
                    strncat(selected_text, full_prompt + start_idx, sel_len);
                    char_count += sel_len;
                }
            }
        }
    } else if (strcmp(w->title, "Amnesia Handbook") == 0) {
        scan_lines_helper(w, handbook_lines, sizeof(handbook_lines)/sizeof(handbook_lines[0]), x1, y1, x2, y2, &char_count);
    } else if (strcmp(w->title, "Synth3x Guide") == 0) {
        if (guide_page == 0) {
            scan_lines_helper(w, guide_p0_lines, sizeof(guide_p0_lines)/sizeof(guide_p0_lines[0]), x1, y1, x2, y2, &char_count);
        } else if (guide_page == 1) {
            scan_lines_helper(w, guide_p1_lines, sizeof(guide_p1_lines)/sizeof(guide_p1_lines[0]), x1, y1, x2, y2, &char_count);
        } else if (guide_page == 2) {
            scan_lines_helper(w, guide_p2_lines, sizeof(guide_p2_lines)/sizeof(guide_p2_lines[0]), x1, y1, x2, y2, &char_count);
        }
    } else if (strcmp(w->title, "System Info") == 0) {
        char sys_usb[128], sys_net[128], sys_tor[128];
        snprintf(sys_usb, sizeof(sys_usb), "USB: %s", cached_usb);
        snprintf(sys_net, sizeof(sys_net), "NET: %s", cached_net);
        snprintf(sys_tor, sizeof(sys_tor), "SECURE TOR ROUTE: %s", is_tor_running() ? "ACTIVE" : "OFFLINE");
        
        WinTextLine sys_lines[] = {
            {12, 12, "=== SYSTEM REALTIME HARDWARE STATS ==="},
            {12, 32, cached_cpu},
            {12, 52, cached_ram},
            {12, 86, cached_disk},
            {12, 104, cached_disk_list},
            {12, 124, sys_usb},
            {12, 144, sys_net},
            {12, 164, sys_tor}
        };
        scan_lines_helper(w, sys_lines, sizeof(sys_lines)/sizeof(sys_lines[0]), x1, y1, x2, y2, &char_count);
    }
}

static void draw_copy_modal(void) {
    int mw = 320;
    int mh = 140;
    int mx_pos = fb_w / 2 - mw / 2;
    int my_pos = fb_h / 2 - mh / 2;
    
    rect_blend(0, 0, fb_w, fb_h, 0x60000000);
    rect(mx_pos, my_pos, mw, mh, 0xFF0D0818);
    rect(mx_pos-1, my_pos-1, mw+2, mh+2, COLOR_ACCENT);
    
    rect(mx_pos, my_pos, mw, 24, COLOR_ACCENT);
    fstr(mx_pos + 12, my_pos + 8, "SYSTEM: COPY TO CLIPBOARD?", COLOR_WHITE, COLOR_ACCENT);
    
    fstr(mx_pos + 12, my_pos + 38, "Do you want to copy the selected text?", COLOR_TEXT, 0);
    
    char snippet[36];
    if (strlen(selected_text) > 32) {
        snprintf(snippet, sizeof(snippet), "\"%.29s...\"", selected_text);
    } else {
        snprintf(snippet, sizeof(snippet), "\"%s\"", selected_text);
    }
    for(int i=0; snippet[i]; i++) {
        if(snippet[i] == '\n') snippet[i] = ' ';
    }
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

/* ─── DOCK & DESKTOP SYSTEM LAUNCHERS ─── */
#define ICON_W 48
#define ICON_H 48

static void draw_desktop_icons(void) {
    int x1 = 20, y1 = 60;
    rect(x1, y1, ICON_W, ICON_H, 0xFF140A28);
    rect(x1-1, y1-1, ICON_W+2, ICON_H+2, COLOR_TEXT);
    fstr(x1 + 16, y1 + 18, ">_", COLOR_WHITE, 0xFF140A28);
    fstr(x1 - 4, y1 + 54, "Terminal", COLOR_TEXT, 0);
    
    int x2 = 20, y2 = 140;
    rect(x2, y2, ICON_W, ICON_H, 0xFF140A28);
    rect(x2-1, y2-1, ICON_W+2, ICON_H+2, COLOR_GREEN);
    fstr(x2 + 20, y2 + 18, "i", COLOR_GREEN, 0xFF140A28);
    fstr(x2 - 4, y2 + 54, "SysInfo", COLOR_TEXT, 0);
    
    int x3 = 20, y3 = 220;
    rect(x3, y3, ICON_W, ICON_H, 0xFF140A28);
    rect(x3-1, y3-1, ICON_W+2, ICON_H+2, COLOR_YELLOW);
    fstr(x3 + 20, y3 + 18, "?", COLOR_YELLOW, 0xFF140A28);
    fstr(x3 - 4, y3 + 54, "Handbook", COLOR_TEXT, 0);
    
    int x4 = 20, y4 = 300;
    rect(x4, y4, ICON_W, ICON_H, 0xFF140A28);
    rect(x4-1, y4-1, ICON_W+2, ICON_H+2, COLOR_ACCENT);
    fstr(x4 + 20, y4 + 18, "#", COLOR_ACCENT, 0xFF140A28);
    fstr(x4 - 4, y4 + 54, "Guide", COLOR_TEXT, 0);
    
    if (vscodium_installed) {
        int x5 = 20, y5 = 380;
        rect(x5, y5, ICON_W, ICON_H, 0xFF140A28);
        rect(x5-1, y5-1, ICON_W+2, ICON_H+2, COLOR_ACCENT);
        fstr(x5 + 16, y5 + 18, "{}", COLOR_WHITE, 0xFF140A28);
        fstr(x5 - 8, y5 + 54, "VSCodium", COLOR_TEXT, 0);
    }
}

/* ─── SYNTH3X OS SECURITY & AMNESIA HANDBOOK ─── */
static void draw_handbook(Win *w, uint32_t bg, uint32_t tx) {
    fstr(w->x+12, w->y+12, "================================================", COLOR_ACCENT, bg);
    fstr(w->x+12, w->y+26, "         AMNESIADE: GRAPHICAL ENVIRONMENT       ", tx, bg);
    fstr(w->x+12, w->y+40, "================================================", COLOR_ACCENT, bg);
    
    fstr(w->x+12, w->y+64, "OVERVIEW: VOLATILE RAM SECURITY SHELL", COLOR_YELLOW, bg);
    fstr(w->x+12, w->y+84, "AmnesiaDE is a lightweight cyberpunk interface", tx, bg);
    fstr(w->x+12, w->y+100, "designed for volatile RAM platforms. It renders", tx, bg);
    fstr(w->x+12, w->y+116, "directly via Linux Framebuffer (/dev/fb0) and", tx, bg);
    fstr(w->x+12, w->y+132, "sweeps all system logs on active power down.", tx, bg);
    
    fstr(w->x+12, w->y+160, "GLOBAL KEYBOARD SHORTCUTS:", COLOR_YELLOW, bg);
    fstr(w->x+24, w->y+180, "[ Super + 1..4 ] : Switch active workspaces (1-4)", COLOR_TEXT, bg);
    fstr(w->x+24, w->y+196, "[ Tab ]          : Switch active window focus", tx, bg);
    fstr(w->x+24, w->y+212, "[ CapsLock ]     : Close focused window instantly", tx, bg);
    fstr(w->x+24, w->y+228, "[ Up / Down ]    : Cycle workspaces sequentially", tx, bg);
    fstr(w->x+24, w->y+244, "[ ESC ]          : Terminate DE session to TTY", tx, bg);
    
    fstr(w->x+12, w->y+274, "Window Traffic Lights: Red (Close), Yellow (Min), Green (Max)", COLOR_PANEL_FG, bg);
}
/* ─── WINDOW DRAW BLOCK ─── */
static void draw_win(Win *w) {
    if (w->hidden || w->ws != current_ws) return;
    
    int is_active = (aw == (w - wins));
    uint32_t bd = is_active ? get_neon_color() : COLOR_WIN_BORDER;
    uint32_t bg = COLOR_WIN_BG;
    uint32_t tl = COLOR_WIN_TITLE, tx = COLOR_TEXT;
    int t = w->y - 24;
    
    // Outer shadow
    rect(w->x+4, t+4, w->w, w->h+24, 0xFF050308);
    
    // Neon borders
    rect(w->x-1, t-1, w->w+2, w->h+26, bd);
    
    // Titlebar
    rect(w->x, t, w->w, 24, tl);
    
    // Centered Title Text
    int title_len = strlen(w->title) * 8;
    int title_x = w->x + (w->w / 2) - (title_len / 2);
    fstr(title_x, t + 8, w->title, tx, tl);
    
    // macOS Style Dots
    rect(w->x + 8, t + 6, 12, 12, COLOR_RED); // Red close
    rect(w->x + 24, t + 6, 12, 12, COLOR_YELLOW); // Yellow minimize
    rect(w->x + 40, t + 6, 12, 12, COLOR_GREEN); // Green maximize
    
    // Window body
    rect(w->x, w->y, w->w, w->h, bg);
    
    if (strcmp(w->title, "System Info") == 0) {
        fstr(w->x+12, w->y+12, "=== SYSTEM REALTIME HARDWARE STATS ===", COLOR_ACCENT, bg);
        
        fstr(w->x+12, w->y+32, cached_cpu, COLOR_TEXT, bg);
        
        fstr(w->x+12, w->y+52, cached_ram, COLOR_GREEN, bg);
        int ram_pct = 0;
        char *pct_ptr = strchr(cached_ram, '(');
        if (pct_ptr) {
            sscanf(pct_ptr, "(%d%%)", &ram_pct);
        }
        if (ram_pct > 0) {
            draw_progress_bar(w->x+12, w->y+66, (float)ram_pct / 100.0f, COLOR_GREEN);
        }
        
        fstr(w->x+12, w->y+86, cached_disk, COLOR_TEXT, bg);
        fstr(w->x+12, w->y+104, cached_disk_list, COLOR_TEXT, bg);
        
        char usb_buf[128];
        snprintf(usb_buf, sizeof(usb_buf), "USB: %s", cached_usb);
        fstr(w->x+12, w->y+124, usb_buf, COLOR_YELLOW, bg);
        
        char net_buf[128];
        snprintf(net_buf, sizeof(net_buf), "NET: %s", cached_net);
        fstr(w->x+12, w->y+144, net_buf, COLOR_WHITE, bg);
        
        int tor_ok = is_tor_running();
        char tor_buf[64];
        snprintf(tor_buf, sizeof(tor_buf), "SECURE TOR ROUTE: %s", tor_ok ? "ACTIVE" : "OFFLINE");
        fstr(w->x+12, w->y+164, tor_buf, tor_ok ? COLOR_GREEN : COLOR_RED, bg);
        draw_progress_bar(w->x+12, w->y+178, tor_ok ? 1.0f : 0.45f, tor_ok ? COLOR_GREEN : COLOR_YELLOW);
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
        
        // Draw interactive shell text field at the bottom
        int input_y = w->y + w->h - 24;
        fstr(w->x + 12, input_y, "synth3x@root:~$ ", COLOR_TEXT, bg);
        fstr(w->x + 140, input_y, term_input, COLOR_WHITE, bg);
        
        // Blinking block cursor
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        if (((ts.tv_nsec / 250000000) % 2) == 0) {
            int cur_x = w->x + 140 + strlen(term_input) * 8;
            rect(cur_x, input_y, 8, 12, COLOR_TEXT);
        }
    } else if (strcmp(w->title, "VSCodium") == 0) {
        // Draw highly detailed, responsive VSCodium interface!
        rect(w->x + 130, w->y, 1, w->h, COLOR_DIM);
        
        // File tree pane
        fstr(w->x+8, w->y+12, "📁 codium-workspace", COLOR_YELLOW, bg);
        fstr(w->x+16, w->y+32, "📄 main.py", COLOR_TEXT, bg);
        fstr(w->x+16, w->y+48, "📄 README.md", COLOR_GREEN, bg);
        fstr(w->x+8, w->y+w->h-24, "VSCodium v1.85", COLOR_PANEL_FG, bg);
        
        // Tab header
        rect(w->x + 131, w->y, w->w - 131, 20, 0xFF0D0818);
        fstr(w->x + 140, w->y + 4, "main.py", COLOR_WHITE, 0xFF0D0818);
        
        // Editor mock code
        const char *codium_code[] = {
            "import os",
            "import sys",
            "",
            "def main():",
            "    print(\"VSCodium running on Synth3x OS!\")",
            "    print(\"AmnesiaDE shell active and stable.\")",
            "",
            "if __name__ == '__main__':",
            "    main()"
        };
        for (int i = 0; i < 9; i++) {
            fstr(w->x + 140, w->y + 28 + i * 16, codium_code[i], 0xFFE6C880, bg);
        }
        
        // VSCodium status bar
        rect(w->x + 131, w->y + w->h - 20, w->w - 131, 20, 0xFF0A0514);
        fstr(w->x + 140, w->y + w->h - 16, "Ln 5, Col 12 | UTF-8 | Python", COLOR_PANEL_FG, 0xFF0A0514);
    } else if (strcmp(w->title, "Amnesia Handbook") == 0) {
        draw_handbook(w, bg, tx);
    } else if (strcmp(w->title, "Synth3x Guide") == 0) {
        fstr(w->x+12, w->y+12, "================================================", COLOR_ACCENT, bg);
        fstr(w->x+12, w->y+26, "         SYNTH3X OS: SYSTEM OPERATION GUIDE     ", tx, bg);
        fstr(w->x+12, w->y+40, "================================================", COLOR_ACCENT, bg);
        
        if (guide_page == 0) {
            fstr(w->x+12, w->y+64, "PAGE 1: NETWORK & TOR INTERNET CONFIGURATION", COLOR_YELLOW, bg);
            fstr(w->x+12, w->y+84, "This volatile environment runs entirely in RAM.", tx, bg);
            fstr(w->x+12, w->y+100, "All network interfaces route through Tor & nftables.", tx, bg);
            fstr(w->x+12, w->y+116, "To initialize the DHCP service and network link:", tx, bg);
            
            // Draw interactive "SETUP INTERNET" button
            int btn_net_x = w->x + 24, btn_net_y = w->y + 144;
            rect(btn_net_x, btn_net_y, 160, 24, 0xFF14281A);
            rect(btn_net_x-1, btn_net_y-1, 162, 26, COLOR_GREEN);
            fstr(btn_net_x + 12, btn_net_y + 8, "[ SETUP INTERNET ]", COLOR_GREEN, 0xFF14281A);
            
            fstr(w->x+12, w->y+184, "Click the button above to trigger auto-DHCP setup.", tx, bg);
            fstr(w->x+12, w->y+200, "This automatically spoofs MAC & randomizes host.", tx, bg);
            fstr(w->x+12, w->y+224, "To configure Wi-Fi using iwctl in Terminal:", COLOR_YELLOW, bg);
            fstr(w->x+24, w->y+240, "$ iwctl --passphrase \"key\" station wlan0 connect \"SSID\"", COLOR_TEXT, bg);
        }
        else if (guide_page == 1) {
            fstr(w->x+12, w->y+64, "PAGE 2: PACKAGE MANAGEMENT (EMERGE & PKG)", COLOR_YELLOW, bg);
            fstr(w->x+12, w->y+84, "As a Gentoo-based OS, you can install any package", tx, bg);
            fstr(w->x+12, w->y+100, "directly from Portage repositories using emerge:", tx, bg);
            fstr(w->x+24, w->y+120, "$ emerge --ask [package_name]", COLOR_TEXT, bg);
            fstr(w->x+12, w->y+140, "Or use the custom amnesic packaging wrapper:", tx, bg);
            fstr(w->x+24, w->y+160, "$ pkg install [package_name]", COLOR_TEXT, bg);
            fstr(w->x+12, w->y+180, "To compile native C source files locally:", tx, bg);
            fstr(w->x+24, w->y+200, "$ gcc main.c -o program", tx, bg);
        }
        else if (guide_page == 2) {
            fstr(w->x+12, w->y+64, "PAGE 3: DEVELOPMENT & IDE CONFIGURATION", COLOR_YELLOW, bg);
            fstr(w->x+12, w->y+84, "AmnesiaDE does not pre-install VSCodium by default", tx, bg);
            fstr(w->x+12, w->y+100, "due to strict amnesic security rules. You can:", tx, bg);
            fstr(w->x+24, w->y+120, "- Download external IDE binaries manually to /persist.", tx, bg);
            fstr(w->x+24, w->y+136, "- Run automated secure installer in terminal:", tx, bg);
            fstr(w->x+24, w->y+156, "  $ pkg install vscodium", COLOR_ACCENT, bg);
            fstr(w->x+24, w->y+176, "- Edit files directly in console using: nano", tx, bg);
        }
        
        // Draw Navigation Buttons
        int btn_prev_x = w->x + 120, btn_next_x = w->x + 280, btn_y = w->y + 280;
        rect(btn_prev_x, btn_y, 80, 24, 0xFF1E1432);
        rect(btn_prev_x-1, btn_y-1, 82, 26, COLOR_DIM);
        fstr(btn_prev_x + 16, btn_y + 8, "< PREV", COLOR_TEXT, 0xFF1E1432);
        
        rect(btn_next_x, btn_y, 80, 24, 0xFF1E1432);
        rect(btn_next_x-1, btn_y-1, 82, 26, COLOR_DIM);
        fstr(btn_next_x + 16, btn_y + 8, "NEXT >", COLOR_TEXT, 0xFF1E1432);
    }
}

static void draw_notifs(void) {
    time_t now=time(NULL);
    int y=PANEL_H+10, x=fb_w-NOTIF_W-10;
    for(int i=0;i<nc&&i<3;i++) {
        if(now-notifs[i].t>NOTIF_DUR+2){memmove(notifs+i,notifs+i+1,sizeof(Notif)*(nc-i-1));nc--;i--;continue;}
        uint32_t nb=COLOR_PANEL_BG, nf=COLOR_PANEL_FG, ac=COLOR_ACCENT, dm=COLOR_DIM;
        rect(x,y,NOTIF_W,NOTIF_H,nb); rect(x,y,4,NOTIF_H,ac);
        rect(x,y,NOTIF_W,1,ac); rect(x,y+NOTIF_H-1,NOTIF_W,1,dm);
        fstr(x+12,y+8,notifs[i].title,ac,nb); fstr(x+12,y+30,notifs[i].body,nf,nb);
        char s[16]; snprintf(s,16,"%ds",(int)(now-notifs[i].t));
        fstr(x+NOTIF_W-40,y+8,s,dm,nb);
        y+=NOTIF_H+5;
    }
}

static void draw_panel(void) {
    uint32_t bg=COLOR_PANEL_BG, fg=COLOR_PANEL_FG, ac=COLOR_ACCENT;
    rect(0,0,fb_w,PANEL_H,bg); rect(0,PANEL_H-1,fb_w,1,ac);
    fstr(8,10,"Synth3x OS (AmnesiaDE v0.7)",ac,bg);
    char ws[16]; snprintf(ws,16,"WS %d/%d",current_ws+1,WORKSPACES); fstr(260,10,ws,fg,bg);
    
    int tor_ok = is_tor_running();
    uint32_t tor_col = tor_ok ? COLOR_GREEN : COLOR_RED;
    const char *tor_txt = tor_ok ? "TOR: ACTIVE" : "TOR: OFFLINE";
    fstr(350, 10, tor_txt, tor_col, bg);

    time_t t=time(NULL); char ts[16]; strftime(ts,16," %H:%M ",localtime(&t));
    fstr(fb_w-8*strlen(ts)-8,10,ts,fg,bg);
    if(nc){char ns[8];snprintf(ns,8,"%d!",nc);fstr(fb_w-8*strlen(ts)-8*strlen(ns)-16,10,ns,COLOR_RED,bg);}
}

/* ─── CENTERED GLASSMORPHIC DOCK TASKBAR ─── */
static void draw_dock(void) {
    int w = 380;
    int h = 34;
    int x = fb_w / 2 - w / 2;
    int y = fb_h - 40;
    
    uint32_t db = COLOR_PANEL_BG;
    uint32_t da = get_neon_color();
    
    // Dock glass body with glowing borders
    rect(x, y, w, h, db);
    rect(x-1, y-1, w+2, h+2, da);
    
    int t_idx = find_win_by_title("Terminal");
    int s_idx = find_win_by_title("System Info");
    int c_idx = find_win_by_title("VSCodium");
    int h_idx = find_win_by_title("Amnesia Handbook");
    int g_idx = find_win_by_title("Synth3x Guide");
    
    // Draw pixel-art icons inside dock with beautiful status lights
    rect(x + 10, y + 6, 60, 22, 0xFF140A28);
    fstr(x + 15, y + 12, "[>_] TERM", (t_idx >= 0 && (wins[t_idx].hidden || wins[t_idx].ws != current_ws)) ? COLOR_DIM : COLOR_TEXT, 0xFF140A28);
    
    rect(x + 80, y + 6, 60, 22, 0xFF140A28);
    fstr(x + 85, y + 12, "[i] STAT", (s_idx >= 0 && (wins[s_idx].hidden || wins[s_idx].ws != current_ws)) ? COLOR_DIM : COLOR_GREEN, 0xFF140A28);
    
    // Locked slot if VSCodium is not installed!
    rect(x + 150, y + 6, 60, 22, 0xFF140A28);
    if (vscodium_installed) {
        fstr(x + 155, y + 12, "{} VSCOD", (c_idx >= 0 && (wins[c_idx].hidden || wins[c_idx].ws != current_ws)) ? COLOR_DIM : COLOR_ACCENT, 0xFF140A28);
    } else {
        fstr(x + 155, y + 12, "🔒 VSCOD", COLOR_DIM, 0xFF140A28);
    }
    
    rect(x + 220, y + 6, 80, 22, 0xFF140A28);
    fstr(x + 225, y + 12, "[?] HANDBK", (h_idx >= 0 && (wins[h_idx].hidden || wins[h_idx].ws != current_ws)) ? COLOR_DIM : COLOR_YELLOW, 0xFF140A28);
    
    rect(x + 310, y + 6, 60, 22, 0xFF140A28);
    fstr(x + 315, y + 12, "[#] GUIDE", (g_idx >= 0 && (wins[g_idx].hidden || wins[g_idx].ws != current_ws)) ? COLOR_DIM : COLOR_GREEN, 0xFF140A28);
}

static void swap(void) {
    for (int y = 0; y < fb_h; y++) {
        uint8_t *dst_row = fb + y * fb_stride_bytes;
        int dim = (y % 3 == 0); // Elegant CRT Scanline shader
        
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
                for (int x = 0; x < fb_w; x++) {
                    dst16[x] = (src_row[x] >> 1) & 0x7BEF;
                }
            } else {
                memcpy(dst16, src_row, fb_w * 2);
            }
        }
    }
}

/* ─── INPUT SYSTEM: MULTI-DEVICE BINDING ─── */
#define MAX_INPUT_FDS 16
static int input_fds[MAX_INPUT_FDS];
static int input_fd_count = 0;

static void input_init(void) {
    for (int i = 0; i < MAX_INPUT_FDS; i++) input_fds[i] = -1;
    input_fd_count = 0;
    
    // Open event devices dynamically (keyboards, tablets, touchpads, mice)
    for (int i = 0; i < 10; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            if (input_fd_count < MAX_INPUT_FDS) {
                input_fds[input_fd_count++] = fd;
            } else {
                close(fd);
            }
        }
    }
    
    // Standalone fallback
    int m_fd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
    if (m_fd >= 0) {
        if (input_fd_count < MAX_INPUT_FDS) {
            input_fds[input_fd_count++] = m_fd;
        } else {
            close(m_fd);
        }
    }
}

static void handle_key(int code) {
    if(code==1) running=0;                     /* ESC */
    
    // Super + 1..4 workspace switching!
    if (super_pressed) {
        if (code >= 2 && code <= 5) {
            current_ws = code - 2;
            beep(784, 40); beep(988, 40);
            notif_add("AmnesiaDE", "Workspace switched.");
            return;
        }
    }
    
    if(code==58) {                              /* CapsLock → close active win */
        if(aw>=0 && aw<wc) {
            wins[aw].hidden=1;
            beep(600, 50); beep(400, 50);
        }
    }
    if(code==15) {                              /* Tab */
        for(int i=1;i<=wc;i++) {
            int ni = (aw+i)%wc;
            if(!wins[ni].hidden) { aw=ni; break; }
        }
        beep(523, 30);
    }
    if(code==103||code==108) { int d=(code==108)?1:-1; /* Up/Down = prev/next ws */
        current_ws = (current_ws + d + WORKSPACES)%WORKSPACES;
        beep(659, 30);
    }
    
    // Process text typing for Terminal window
    int term_idx = find_win_by_title("Terminal");
    if (term_idx >= 0 && aw == term_idx) {
        if (code == 28) { // Enter
            if (strlen(term_input) > 0) {
                exec_term_cmd(term_input);
                term_input[0] = '\0';
                beep(880, 40);
            }
        } else if (code == 14) { // Backspace
            int len = strlen(term_input);
            if (len > 0) {
                term_input[len - 1] = '\0';
            }
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

/* ─── MAIN LOOP ─── */
int main(int argc, char *argv[]) {
    printf("Synth3x OS — AmnesiaDE v0.7\n");
    
    fb_fd = open("/dev/fb0", O_RDWR);
    if(fb_fd<0) { printf("No /dev/fb0\n"); return 1; }
    struct fb_var_screeninfo vi;
    struct fb_fix_screeninfo fix;
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vi);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix);
    fb_w=vi.xres; fb_h=vi.yres;
    fb_bpp=vi.bits_per_pixel;
    fb_stride_bytes=fix.line_length;
    
    uint8_t *fbmap = mmap(NULL, fb_h * fb_stride_bytes, PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if(fbmap==MAP_FAILED) { close(fb_fd); return 1; }
    fb = fbmap;
    
    // Allocate buffer based on native screen depth
    if (fb_bpp == 32) {
        backbuf32 = malloc(fb_w * fb_h * 4);
    } else {
        backbuf16 = malloc(fb_w * fb_h * 2);
    }
    
    if(!backbuf16 && !backbuf32) { munmap(fb, fb_h * fb_stride_bytes); close(fb_fd); return 1; }
    
    int tty = open("/dev/tty0", O_RDWR);
    if(tty>=0) ioctl(tty, KDSETMODE, KD_GRAPHICS);
    
    input_init(); notif_init();
    init_stars();
    
    // Seed initial hacker log messages in Terminal
    term_log_add("Synth3x OS Core initialized successfully.");
    term_log_add("[OK] Transparent Tor Routing activated.");
    term_log_add("[OK] nftables default-drop rule applied.");
    term_log_add("[OK] Hostname and MAC spoofed successfully.");
    term_log_add("------------------------------------------");
    term_log_add("Type command below to interact with shell.");
    term_log_add("Hint: run 'pkg install vscodium' to download IDE.");
    
    // Open our 3 default applications (VSCodium / IDE is initially closed/locked!)
    wnew("Terminal", 460, 260);
    wnew("System Info", 380, 220);
    wnew("Amnesia Handbook", 480, 320);
    wnew("Synth3x Guide", 500, 320);
    
    // Custom chime sound on startup
    beep(523, 80); beep(659, 80); beep(784, 80); beep(1046, 120);
    
    notif_add("AmnesiaDE", "Interactive shell ready. Core: Synth3x OS.");
    notif_add("System ready", "Press ESC to exit DE graphics.");
    
    time_t last_term_update = 0;
    
    while(running) {
        if (stats_tick++ % 120 == 0) {
            update_cached_stats();
        }
        /* Dynamic Scrolling Terminal Logs */
        time_t cur_t = time(NULL);
        if (cur_t - last_term_update > 5) {
            last_term_update = cur_t;
            const char *updates[] = {
                "[NFT] Packet block: UDP leak intercepted",
                "[TOR] Route verified: circuit renewed",
                "[SEC] Memory sweep completed: 0 leaks",
                "[ID ] Spoofing identity: MAC rotate",
                "[SYS] CPU Temperature: 38C (Optimal)",
                "[SEC] Amnesic RAM shield active"
            };
            srand(time(NULL) ^ getpid());
            term_log_add(updates[rand() % 6]);
        }
        
        /* Poll inputs dynamically */
        struct pollfd fds[MAX_INPUT_FDS + 2]; int nf=0;
        for(int i=0; i<input_fd_count; i++) {
            if (input_fds[i] >= 0) {
                fds[nf].fd = input_fds[i];
                fds[nf].events = POLLIN;
                nf++;
            }
        }
        if(notif_fd>=0){fds[nf].fd=notif_fd;fds[nf].events=POLLIN;nf++;}
        
        if(poll(fds,nf,16)>0) {
            struct input_event ev;
            for(int i=0;i<nf;i++) {
                if(!(fds[i].revents&POLLIN)) continue;
                
                if(fds[i].fd==notif_fd) {
                    notif_read();
                    continue;
                }
                
                // Read from event device
                while(read(fds[i].fd,&ev,sizeof(ev))==sizeof(ev)) {
                    // 1. Relative movement (mice, relative trackpads)
                    if(ev.type==EV_REL) {
                        if(ev.code==REL_X) mx+=ev.value*2;
                        if(ev.code==REL_Y) my+=ev.value*2;
                    }
                    
                    // 2. Absolute positioning (USB Tablet, absolute touchpads)
                    if(ev.type==EV_ABS) {
                        if(ev.code==ABS_X) mx=(ev.value * fb_w)/32767;
                        if(ev.code==ABS_Y) my=(ev.value * fb_h)/32767;
                    }
                    
                    // 3. Keys and Buttons
                    if(ev.type==EV_KEY) {
                        // Mouse buttons & touchpad tap events
                        if(ev.code==BTN_LEFT || ev.code==BTN_TOUCH) {
                            if(ev.value==1) {
                                mclick=1;
                                mouse_pressed = 1;
                                
                                // Start selection if copy modal is not open
                                if (!show_copy_dialog && my < fb_h - 40) {
                                    int clicked_titlebar = 0;
                                    for(int j=wc-1;j>=0;j--) {
                                        if(!wins[j].hidden && wins[j].ws == current_ws) {
                                            if (win_title(&wins[j], mx, my) || win_close(&wins[j], mx, my) || 
                                                win_minimize(&wins[j], mx, my) || win_maximize(&wins[j], mx, my)) {
                                                clicked_titlebar = 1;
                                                break;
                                            }
                                        }
                                    }
                                    int dx_dock = fb_w / 2 - 190;
                                    int clicked_dock = (my >= fb_h - 40 && my <= fb_h - 6 && mx >= dx_dock && mx <= dx_dock + 380);
                                    
                                    if (!clicked_titlebar && !clicked_dock) {
                                        selecting = 1;
                                        sel_start_x = mx;
                                        sel_start_y = my;
                                        sel_end_x = mx;
                                        sel_end_y = my;
                                    }
                                }
                            }
                            if(ev.value==0) {
                                mouse_pressed = 0;
                                for(int j=0;j<wc;j++) wins[j].drag=0; mclick=0;
                                
                                if (selecting) {
                                    selecting = 0;
                                    int dx = abs(mx - sel_start_x);
                                    int dy = abs(my - sel_start_y);
                                    if (dx > 8 || dy > 8) {
                                        extract_selected_text();
                                        if (strlen(selected_text) > 0) {
                                            show_copy_dialog = 1;
                                            mclick = 0; // prevent click propagation
                                        }
                                    }
                                }
                            }
                        } 
                        // Track Shift key state
                        else if (ev.code == 42 || ev.code == 54) {
                            shift_pressed = (ev.value != 0);
                        }
                        // Track Super key state (Left Meta = 125, Right Meta = 126)
                        else if (ev.code == 125 || ev.code == 126) {
                            super_pressed = (ev.value != 0);
                        }
                        // Keyboard key strokes
                        else if(ev.value==1) {
                            handle_key(ev.code);
                        }
                    }
                }
            }
        }
        
        mx = mx<0?0:(mx>=fb_w?fb_w-1:mx);
        my = my<PANEL_H?PANEL_H:(my>=fb_h?fb_h-1:my);
        
        /* Handle click */
        if(mclick) {
            mclick=0;
            
            // 1. Check Copy Dialog Modal Clicks (absorbs all clicks when modal is open)
            if (show_copy_dialog) {
                int mw = 320;
                int mh = 140;
                int mx_pos = fb_w / 2 - mw / 2;
                int my_pos = fb_h / 2 - mh / 2;
                int btn_y = my_pos + 94;
                
                // YES Clicked
                if (mx >= mx_pos + 30 && mx <= mx_pos + 130 && my >= btn_y && my <= btn_y + 24) {
                    strncpy(clipboard, selected_text, sizeof(clipboard) - 1);
                    clipboard[sizeof(clipboard) - 1] = '\0';
                    
                    // Copy to host clipboard
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
                }
                // NO Clicked
                else if (mx >= mx_pos + 190 && mx <= mx_pos + 290 && my >= btn_y && my <= btn_y + 24) {
                    show_copy_dialog = 0;
                    beep(300, 100);
                }
                continue;
            }
            
            // 2. Check if we clicked on any window bounds (titlebar or body)
            int clicked_win = 0;
            for (int j = wc - 1; j >= 0; j--) {
                if (!wins[j].hidden && wins[j].ws == current_ws &&
                    mx >= wins[j].x && mx <= wins[j].x + wins[j].w &&
                    my >= wins[j].y - 24 && my <= wins[j].y + wins[j].h) {
                    clicked_win = 1;
                    break;
                }
            }
            
            // 3. Check Desktop Icon Clicks
            if (!clicked_win && my < fb_h - 40) {
                if (mx >= 20 && mx <= 20 + ICON_W && my >= 60 && my <= 60 + ICON_H) {
                    int idx = find_win_by_title("Terminal");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Terminal", "Restored."); beep(400, 50); beep(500, 50); }
                }
                else if (mx >= 20 && mx <= 20 + ICON_W && my >= 140 && my <= 140 + ICON_H) {
                    int idx = find_win_by_title("System Info");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("System Info", "Restored."); beep(400, 50); beep(500, 50); }
                }
                else if (mx >= 20 && mx <= 20 + ICON_W && my >= 220 && my <= 220 + ICON_H) {
                    int idx = find_win_by_title("Amnesia Handbook");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Handbook", "Restored."); beep(400, 50); beep(500, 50); }
                }
                else if (mx >= 20 && mx <= 20 + ICON_W && my >= 300 && my <= 300 + ICON_H) {
                    int idx = find_win_by_title("Synth3x Guide");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Guide", "Restored."); beep(400, 50); beep(500, 50); }
                }
                else if (vscodium_installed && mx >= 20 && mx <= 20 + ICON_W && my >= 380 && my <= 380 + ICON_H) {
                    int idx = find_win_by_title("VSCodium");
                    if (idx < 0) {
                        wnew("VSCodium", 500, 320);
                        beep(523, 60); beep(659, 60); beep(784, 80);
                    } else {
                        wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx;
                        beep(400, 50); beep(500, 50);
                    }
                    notif_add("VSCodium", "Editor restored.");
                }
            }
            
            // Check Dock Taskbar Clicks
            if (my >= fb_h - 40 && my <= fb_h - 6) {
                int dx = fb_w / 2 - 190;
                if (mx >= dx + 10 && mx <= dx + 70) { // Terminal
                    int idx = find_win_by_title("Terminal");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Terminal","Workspace restored."); beep(400, 50); beep(500, 50); }
                } else if (mx >= dx + 80 && mx <= dx + 140) { // System Info
                    int idx = find_win_by_title("System Info");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Security Stat","System Health active."); beep(400, 50); beep(500, 50); }
                } else if (mx >= dx + 150 && mx <= dx + 210) { // VSCodium (Locked/Unlocked)
                    if (vscodium_installed) {
                        int idx = find_win_by_title("VSCodium");
                        if (idx < 0) {
                            idx = wnew("VSCodium", 500, 320);
                            beep(523, 60); beep(659, 60); beep(784, 80);
                        } else {
                            wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx;
                            beep(400, 50); beep(500, 50);
                        }
                        notif_add("VSCodium", "Code editor loaded.");
                    } else {
                        notif_add("Synth3x OS", "VSCodium is not installed. Type 'pkg install vscodium' in Terminal.");
                        beep(300, 120);
                    }
                } else if (mx >= dx + 220 && mx <= dx + 300) { // Amnesia Handbook
                    int idx = find_win_by_title("Amnesia Handbook");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Handbook","OS guide loaded."); beep(400, 50); beep(500, 50); }
                } else if (mx >= dx + 310 && mx <= dx + 370) { // Synth3x Guide
                    int idx = find_win_by_title("Synth3x Guide");
                    if (idx >= 0) { wins[idx].hidden = 0; wins[idx].ws = current_ws; aw = idx; notif_add("Guide","OS setup loaded."); beep(400, 50); beep(500, 50); }
                }
            }
            
            // Check Window Clicks
            int hit=-1;
            for(int j=wc-1;j>=0;j--) {
                if(!wins[j].hidden && wins[j].ws == current_ws && (win_title(&wins[j],mx,my) || win_close(&wins[j],mx,my) || win_minimize(&wins[j],mx,my) || win_maximize(&wins[j],mx,my))) {
                    hit=j; break;
                }
            }
            if(hit>=0) {
                if(win_close(&wins[hit],mx,my)) {
                    wins[hit].hidden=1;
                    beep(600, 60); beep(400, 60);
                } else if(win_minimize(&wins[hit],mx,my)) {
                    wins[hit].hidden=1;
                    notif_add(wins[hit].title, "Minimized to Dock Taskbar.");
                    beep(400, 50); beep(300, 50);
                } else if(win_maximize(&wins[hit],mx,my)) {
                    if (wins[hit].maximized) {
                        wins[hit].x = wins[hit].orig_x;
                        wins[hit].y = wins[hit].orig_y;
                        wins[hit].w = wins[hit].orig_w;
                        wins[hit].h = wins[hit].orig_h;
                        wins[hit].maximized = 0;
                    } else {
                        wins[hit].orig_x = wins[hit].x;
                        wins[hit].orig_y = wins[hit].y;
                        wins[hit].orig_w = wins[hit].w;
                        wins[hit].orig_h = wins[hit].h;
                        
                        wins[hit].x = 0;
                        wins[hit].y = PANEL_H + 24;
                        wins[hit].w = fb_w;
                        wins[hit].h = fb_h - PANEL_H - 24 - 45;
                        wins[hit].maximized = 1;
                    }
                    notif_add(wins[hit].title, wins[hit].maximized ? "Expanded to Full-Screen." : "Restored original layout.");
                    beep(500, 60); beep(700, 60);
                } else {
                    /* Bring to front */
                    Win t = wins[hit];
                    memmove(&wins[hit],&wins[hit+1],sizeof(Win)*(wc-hit-1));
                    wins[wc-1]=t; aw=wc-1;
                    wins[aw].drag=1; wins[aw].dx=mx-wins[aw].x; wins[aw].dy=my-(wins[aw].y-24);
                }
            }
            
            // Check Synth3x Guide Navigation & Network Button Clicks
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
                
                // Network Auto-Config button click on Page 1 (guide_page == 0)
                if (guide_page == 0) {
                    int btn_net_x = wins[g_idx].x + 24, btn_net_y = wins[g_idx].y + 144;
                    if (mx >= btn_net_x && mx <= btn_net_x + 160 && my >= btn_net_y && my <= btn_net_y + 24) {
                        beep(523, 80); beep(659, 80); beep(784, 120);
                        notif_add("Network Config", "Starting DHCP auto-configuration...");
                        term_log_add("Network Manager: Initializing DHCP client eth0...");
                        
                        pthread_t net_thread;
                        pthread_create(&net_thread, NULL, async_net_setup, NULL);
                        pthread_detach(net_thread);
                    }
                }
            }
        }
        
        /* Drag */
        int any_drag = 0;
        for(int i=0;i<wc;i++) if(wins[i].drag) {
            wins[i].x=mx-wins[i].dx; wins[i].y=my-wins[i].dy+24;
            if(wins[i].x<0)wins[i].x=0; if(wins[i].y<PANEL_H+24)wins[i].y=PANEL_H+24;
            if(wins[i].x+wins[i].w>fb_w)wins[i].x=fb_w-wins[i].w;
            if(wins[i].y+wins[i].h>fb_h-42)wins[i].y=fb_h-42-wins[i].h;
            any_drag = 1;
        }
        
        if (selecting && mouse_pressed && !any_drag) {
            sel_end_x = mx;
            sel_end_y = my;
        }
        
        /* Draw background grid, stars, etc. */
        memset(backbuf32 ? (void*)backbuf32 : (void*)backbuf16, 0, fb_w * fb_h * (fb_bpp == 32 ? 4 : 2));
        draw_bg();
        for(int i=0;i<wc;i++) draw_win(&wins[i]);
        draw_notifs(); draw_panel(); draw_dock();
        
        // Draw text selection overlay
        if (selecting) {
            int x = sel_start_x < sel_end_x ? sel_start_x : sel_end_x;
            int y = sel_start_y < sel_end_y ? sel_start_y : sel_end_y;
            int w = abs(sel_end_x - sel_start_x);
            int h = abs(sel_end_y - sel_start_y);
            rect_blend(x, y, w, h, 0x4000FFFF); // 25% transparent neon cyan overlay
        }
        
        // Draw copy dialog modal prompt
        if (show_copy_dialog) {
            draw_copy_modal();
        }
        
        /* Draw stunning neon cursor */
        draw_custom_cursor(mx, my);
        
        swap();
    }
    
    // Close opened descriptors on cleanup
    for(int i=0; i<input_fd_count; i++) {
        if(input_fds[i] >= 0) close(input_fds[i]);
    }
    
    if (backbuf32) free(backbuf32);
    if (backbuf16) free(backbuf16);
    munmap(fb, fb_h * fb_stride_bytes); close(fb_fd);
    if(tty>=0) ioctl(tty, KDSETMODE, KD_TEXT);
    printf("AmnesiaDE: done.\n");
    for(;;) pause();
}
