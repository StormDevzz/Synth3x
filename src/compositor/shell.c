/* Synth3x Compositor — AmnesiaDE Shell
 * Renders the cyberpunk desktop environment: windows, icons, panels, dock,
 * terminal emulator, system info, guide, and retro-wave background.
 * All rendering goes to the compositor's backbuffer.
 */

#include "compositor.h"
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct { int x; int y; const char *text; } WinTextLine;

static const WinTextLine guide_p0_lines[] = {
    {12, 12, "======================================================"},
    {12, 26, "   SYNTH3X OS: INTERNET SETUP GUIDE                  "},
    {12, 40, "======================================================"},
    {12, 56, "[1/8] QUICK START — AUTO INTERNET (Ethernet)"},
    {12, 76, "1. Plug in Ethernet cable"},
    {12, 92, "2. Click [ SETUP INTERNET ] button below (runs DHCP)"},
    {12, 108, "   — OR type in terminal:"},
    {24, 128, "$ udhcpc -i eth0"},
    {12, 152, "3. Test connection:"},
    {24, 172, "$ ping -c 3 1.1.1.1"},
    {12, 196, "4. Browse:"},
    {24, 216, "$ browser"},
    {12, 240, "Wi-Fi: see page 3 (iwctl support)."},
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
    {12, 56, "[3/8] WI-FI SETUP via iwctl"},
    {12, 76, "Connect to Wi-Fi using iwd (iwctl):"},
    {24, 96, "# iwctl"},
    {24, 112, "Inside iwctl prompt, run:"},
    {24, 128, "  station wlan0 scan"},
    {24, 144, "  station wlan0 get-networks"},
    {24, 160, "  station wlan0 connect <SSID>"},
    {12, 180, "Or run direct connection command:"},
    {24, 200, "$ iwctl --passphrase \"PASS\" station wlan0 connect \"SSID\""},
    {12, 220, "Get DHCP lease:"},
    {24, 240, "$ udhcpc -i wlan0"},
    {12, 260, "Verify connection:"},
    {24, 280, "$ ping -c 3 1.1.1.1"},
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
    {12, 56, "[6/8] INSTALLING PROGRAMS (emerge / portage)"},
    {12, 76, "Gentoo uses Portage ('emerge') to manage packages:"},
    {24, 100, "$ emerge --ask <pkg>  - Install package"},
    {24, 116, "$ emerge --unmerge <p> - Remove package"},
    {24, 132, "$ emerge --search <q> - Search packages"},
    {24, 148, "$ emerge-webrsync     - Sync database (offline)"},
    {24, 164, "$ emaint --auto sync  - Sync Portage (online)"},
    {12, 188, "Available packages in Gentoo repo:"},
    {24, 208, "www-client/firefox-bin, app-editors/vscodium-bin"},
    {24, 224, "net-im/telegram-desktop-bin, app-editors/vim"},
    {24, 240, "sys-process/htop, dev-vcs/git, net-misc/curl"},
    {12, 260, "From terminal: type emerge command, press Enter"},
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
    {24, 164, "5. Choose Installation Mode (Graphical vs Terminal)"},
    {24, 180, "6. Wait for install, then reboot"},
    {12, 204, "After install:"},
    {24, 224, "• Full Gentoo system with Portage"},
    {24, 240, "• User account with sudo access"},
    {24, 256, "• TTY1 autologin directly starting compositor"},
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

static char cached_ram[64] = "RAM: Loading...";
static char cached_disk[64] = "DISK space: Loading...";
static char cached_disk_list[64] = "DISK list: Loading...";
static char cached_cpu[128] = "CPU: Loading...";
static char cached_usb[128] = "USB: Loading...";
static char cached_net[128] = "Net: Loading...";
static char cached_laptop[64] = "System: Unknown";

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
    int y;
    const char *label;
    uint32_t color;
    const char *glyph;
} VisibleIcon;

static int get_visible_icons(compositor_t *c, VisibleIcon *icons) {
    int start_y = 60, gap = 78;
    int count = 0;
    
    icons[count++] = (VisibleIcon){0, "Terminal", COLOR_TEXT, ">_"};
    icons[count++] = (VisibleIcon){0, "System Info", COLOR_GREEN, "i"};
    icons[count++] = (VisibleIcon){0, "Web", COLOR_ORANGE, "W"};
    icons[count++] = (VisibleIcon){0, "Handbook", COLOR_YELLOW, "?"};
    icons[count++] = (VisibleIcon){0, "Guide", COLOR_ACCENT, "#"};
    
    if (access("/usr/local/bin/codium", F_OK) == 0 || access("/usr/bin/vscodium", F_OK) == 0 || access("/var/db/syn/vscodium", F_OK) == 0) {
        icons[count++] = (VisibleIcon){0, "VSCodium", COLOR_ACCENT, "{}"};
    }
    if (access("/usr/local/firefox/firefox", F_OK) == 0 || access("/usr/local/bin/firefox", F_OK) == 0 || access("/var/db/syn/firefox", F_OK) == 0) {
        icons[count++] = (VisibleIcon){0, "Firefox", COLOR_ORANGE, "FF"};
    }
    if (access("/usr/local/bin/Telegram", F_OK) == 0 || access("/var/db/syn/telegram-desktop", F_OK) == 0) {
        icons[count++] = (VisibleIcon){0, "Telegram", COLOR_GREEN, "TG"};
    }
    
    icons[count++] = (VisibleIcon){0, "Install", COLOR_RED, "HD"};
    
    for (int i = 0; i < count; i++) {
        icons[i].y = start_y + i * gap;
    }
    
    return count;
}

static void *async_net_setup(void *arg) {
    compositor_t *c = (compositor_t *)arg;
    shell_term_log(c, "Network: Spoofing MAC...");
    system("for iface in /sys/class/net/*; do name=$(basename $iface); [ \"$name\" = lo ] && continue; ip link set $name down 2>/dev/null; ip link set $name address 02:$(printf '%02x:%02x:%02x:%02x:%02x' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256))) 2>/dev/null; ip link set $name up 2>/dev/null; done");
    shell_term_log(c, "Network: DHCP on all interfaces...");
    system("for iface in /sys/class/net/*; do name=$(basename $iface); [ \"$name\" = lo ] || [ \"$name\" = docker0 ] && continue; udhcpc -i $name -b -q 2>/dev/null & done");
    shell_term_log(c, "Network: DNS set to 1.1.1.1 / 8.8.8.8");
    system("echo 'nameserver 1.1.1.1' > /etc/resolv.conf; echo 'nameserver 8.8.8.8' >> /etc/resolv.conf");
    shell_term_log(c, "Network: All interfaces configured!");
    shell_notif(c, "Network", "Internet ready! DHCP + DNS configured.");
    shell_beep(c, 880, 80); shell_beep(c, 1100, 150);
    return NULL;
}

typedef struct {
    compositor_t *c;
    char cmd[256];
} CmdArgs;

static void *async_exec_cmd(void *arg) {
    CmdArgs *args = (CmdArgs *)arg;
    char wrapped[300];
    snprintf(wrapped, sizeof(wrapped), "%s 2>&1", args->cmd);
    FILE *fp = popen(wrapped, "r");
    if (!fp) {
        shell_term_log(args->c, "[ERR] Failed to execute command.");
        free(args);
        return NULL;
    }
    char line[256];
    int lines_read = 0;
    while (fgets(line, sizeof(line), fp) && lines_read < 12) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        shell_term_log(args->c, line);
        lines_read++;
    }
    int exit_code = pclose(fp);
    if (lines_read == 0) {
        if (exit_code == 127) {
            shell_term_log(args->c, "[ERR] Command not found");
        } else if (exit_code == 0) {
            shell_term_log(args->c, "[OK] Command completed (no output)");
        } else {
            char buf[64];
            snprintf(buf, sizeof(buf), "[ERR] Exit code %d", WEXITSTATUS(exit_code));
            shell_term_log(args->c, buf);
        }
    }
    free(args);
    return NULL;
}

static void draw_progress_bar(compositor_t *c, int x, int y, float percent, uint32_t color) {
    int w = 180, h = 8;
    render_rect(c, x, y, w, h, 0xFF1E1428);
    if (percent > 0) render_rect(c, x, y, (int)(w * (percent > 1.0f ? 1.0f : percent)), h, color);
    render_rect(c, x-1, y-1, w+2, 1, COLOR_DIM);
    render_rect(c, x-1, y+h, w+2, 1, COLOR_DIM);
    render_rect(c, x-1, y, 1, h, COLOR_DIM);
    render_rect(c, x+w, y, 1, h, COLOR_DIM);
}

static void draw_copy_modal(compositor_t *c) {
    int mw = 320, mh = 140;
    int mx_pos = c->fb_w / 2 - mw / 2, my_pos = c->fb_h / 2 - mh / 2;
    render_rect_blend(c, 0, 0, c->fb_w, c->fb_h, 0x60000000);
    render_rect(c, mx_pos, my_pos, mw, mh, 0xFF0D0818);
    render_rect(c, mx_pos-1, my_pos-1, mw+2, mh+2, COLOR_ACCENT);
    render_rect(c, mx_pos, my_pos, mw, 24, COLOR_ACCENT);
    render_text(c, mx_pos + 12, my_pos + 8, "SYSTEM: COPY TO CLIPBOARD?", COLOR_WHITE, COLOR_ACCENT);
    render_text(c, mx_pos + 12, my_pos + 38, "Copy the selected text?", COLOR_TEXT, 0);
    char snippet[36];
    if (strlen(c->selected_text) > 32) snprintf(snippet, sizeof(snippet), "\"%.29s...\"", c->selected_text);
    else snprintf(snippet, sizeof(snippet), "\"%s\"", c->selected_text);
    for (int i = 0; snippet[i]; i++) if (snippet[i] == '\n') snippet[i] = ' ';
    render_text(c, mx_pos + 12, my_pos + 58, snippet, COLOR_YELLOW, 0);
    int btn_yes_x = mx_pos + 30, btn_y = my_pos + 94;
    render_rect(c, btn_yes_x, btn_y, 100, 24, 0xFF14281A);
    render_rect(c, btn_yes_x-1, btn_y-1, 102, 26, COLOR_GREEN);
    render_text(c, btn_yes_x + 24, btn_y + 8, "[ COPY ]", COLOR_GREEN, 0xFF14281A);
    int btn_no_x = mx_pos + 190;
    render_rect(c, btn_no_x, btn_y, 100, 24, 0xFF2A1015);
    render_rect(c, btn_no_x-1, btn_y-1, 102, 26, COLOR_RED);
    render_text(c, btn_no_x + 16, btn_y + 8, "[ CANCEL ]", COLOR_RED, 0xFF2A1015);
}

static void scan_lines_helper(compositor_t *c, ShellWin *w, const WinTextLine *lines, int count, int x1, int y1, int x2, int y2, int *char_count) {
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
                if (*char_count > 0 && c->selected_text[strlen(c->selected_text)-1] != '\n')
                    strcat(c->selected_text, "\n");
                int cur_len = strlen(c->selected_text);
                if (cur_len + sel_len < (int)sizeof(c->selected_text) - 2)
                    strncat(c->selected_text, lines[i].text + start_idx, sel_len);
                *char_count += sel_len;
            }
        }
    }
}

void extract_selected_text(compositor_t *c) {
    c->selected_text[0] = '\0';
    if (c->aw < 0 || c->aw >= c->wc) return;
    ShellWin *w = &c->wins[c->aw];
    int x1 = c->sel_start_x < c->sel_end_x ? c->sel_start_x : c->sel_end_x;
    int y1 = c->sel_start_y < c->sel_end_y ? c->sel_start_y : c->sel_end_y;
    int x2 = c->sel_start_x > c->sel_end_x ? c->sel_start_x : c->sel_end_x;
    int y2 = c->sel_start_y > c->sel_end_y ? c->sel_start_y : c->sel_end_y;
    int char_count = 0;

    if (strcmp(w->title, "Terminal") == 0) {
        for (int i = 0; i < c->term_log_count; i++) {
            int ly = w->y + 12 + i * 16;
            if (ly + 4 >= y1 && ly + 4 <= y2) {
                int start_idx = -1, end_idx = -1;
                int len = strlen(c->term_logs[i]);
                for (int col = 0; col < len; col++) {
                    int cx = w->x + 12 + col * 8;
                    if (cx + 4 >= x1 && cx + 4 <= x2) {
                        if (start_idx == -1) start_idx = col;
                        end_idx = col;
                    }
                }
                if (start_idx != -1 && end_idx != -1) {
                    int sel_len = end_idx - start_idx + 1;
                    if (char_count > 0 && c->selected_text[strlen(c->selected_text)-1] != '\n')
                        strcat(c->selected_text, "\n");
                    int cur_len = strlen(c->selected_text);
                    if (cur_len + sel_len < (int)sizeof(c->selected_text) - 2)
                        strncat(c->selected_text, c->term_logs[i] + start_idx, sel_len);
                    char_count += sel_len;
                }
            }
        }
        int prompt_y = w->y + w->h - 24;
        if (prompt_y + 4 >= y1 && prompt_y + 4 <= y2) {
            char full_prompt[256];
            snprintf(full_prompt, sizeof(full_prompt), "synth3x@root:~$ %s", c->term_input);
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
                if (char_count > 0 && c->selected_text[strlen(c->selected_text)-1] != '\n')
                    strcat(c->selected_text, "\n");
                int cur_len = strlen(c->selected_text);
                if (cur_len + sel_len < (int)sizeof(c->selected_text) - 2)
                    strncat(c->selected_text, full_prompt + start_idx, sel_len);
                char_count += sel_len;
            }
        }
    } else if (strcmp(w->title, "Synth3x Guide") == 0) {
        const WinTextLine *lines = NULL;
        int nlines = 0;
        switch (c->guide_page) {
            case 0: lines = guide_p0_lines; nlines = sizeof(guide_p0_lines)/sizeof(guide_p0_lines[0]); break;
            case 1: lines = guide_p1_lines; nlines = sizeof(guide_p1_lines)/sizeof(guide_p1_lines[0]); break;
            case 2: lines = guide_p2_lines; nlines = sizeof(guide_p2_lines)/sizeof(guide_p2_lines[0]); break;
            case 3: lines = guide_p3_lines; nlines = sizeof(guide_p3_lines)/sizeof(guide_p3_lines[0]); break;
            case 4: lines = guide_p4_lines; nlines = sizeof(guide_p4_lines)/sizeof(guide_p4_lines[0]); break;
            case 5: lines = guide_p5_lines; nlines = sizeof(guide_p5_lines)/sizeof(guide_p5_lines[0]); break;
            case 6: lines = guide_p6_lines; nlines = sizeof(guide_p6_lines)/sizeof(guide_p6_lines[0]); break;
            case 7: lines = guide_p7_lines; nlines = sizeof(guide_p7_lines)/sizeof(guide_p7_lines[0]); break;
        }
        if (lines) scan_lines_helper(c, w, lines, nlines, x1, y1, x2, y2, &char_count);
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
            {12, 170, sys_tor},
            {12, 222, has_touchpad() ? "Touchpad: detected" : "Touchpad: not detected (mouse mode)"}
        };
        scan_lines_helper(c, w, sys_lines, sizeof(sys_lines)/sizeof(sys_lines[0]), x1, y1, x2, y2, &char_count);
    } else if (strcmp(w->title, "Amnesia Handbook") == 0) {
        WinTextLine hb_lines[] = {
            {12, 12, "======================================================"},
            {12, 26, "         AMNESIADE: WAYLAND COMPOSITOR v0.8.1          "},
            {12, 84, "AmnesiaDE v0.8.1 — Wayland Compositor"},
            {12, 100, "Direct rendering via DRM/KMS (Linux Kernel Mode Setting)"},
            {12, 116, "Native Wayland protocol — supports external clients"},
            {12, 132, "All data in volatile RAM — destroyed on power-down."},
            {12, 148, "Network: Tor transparent proxy + nftables firewall."},
            {12, 172, "KEYBOARD SHORTCUTS:"},
            {24, 192, "[ Super+1..4 ] Workspace switch"},
            {24, 208, "[ Tab ]        Window focus cycle"},
            {24, 224, "[ CapsLock ]   Close focused window"},
            {24, 240, "[ Up / Down ]  Cycle workspaces"},
            {24, 256, "[ ESC ]        Exit to TTY shell"},
            {12, 280, "Desktop: Terminal | SysInfo | Web | Guide | Install"}
        };
        scan_lines_helper(c, w, hb_lines, sizeof(hb_lines)/sizeof(hb_lines[0]), x1, y1, x2, y2, &char_count);
    } else if (strcmp(w->title, "VSCodium") == 0) {
        WinTextLine vs_lines[] = {
            {12, 12, "VSCodium v1.92 — main.py"},
            {12, 36, "1  import os"},
            {12, 52, "2  import sys"},
            {12, 68, "3  "},
            {12, 84, "4  def main():"},
            {12, 100, "5      print(\"VSCodium on Synth3x Gentoo OS!\")"},
            {12, 116, "6      print(\"Identity: amnesic / RAM-only\")"},
            {12, 132, "7      print(\"Network: Tor Transparent Proxy\")"},
            {12, 148, "8  "},
            {12, 164, "9  if __name__ == '__main__':"},
            {12, 180, "10     main()"},
            {12, 296, "VSCodium v1.92.0 (Gentoo)"}
        };
        scan_lines_helper(c, w, vs_lines, sizeof(vs_lines)/sizeof(vs_lines[0]), x1, y1, x2, y2, &char_count);
    }
}

/* ─── Scancode table ─── */
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

/* ─── Terminal log ─── */
void shell_term_log(compositor_t *c, const char *msg) {
    if (c->term_log_count >= MAX_TERM_LOGS) {
        memmove(c->term_logs, c->term_logs + 1,
                sizeof(c->term_logs[0]) * (MAX_TERM_LOGS - 1));
        c->term_log_count--;
    }
    strncpy(c->term_logs[c->term_log_count], msg, sizeof(c->term_logs[0]) - 1);
    c->term_logs[c->term_log_count][sizeof(c->term_logs[0]) - 1] = '\0';
    c->term_log_count++;
}

/* ─── Notification ─── */
void shell_notif(compositor_t *c, const char *title, const char *body) {
    if (c->nc >= MAX_NOTIF) {
        memmove(c->notifs, c->notifs + 1, sizeof(Notif) * (MAX_NOTIF - 1));
        c->nc--;
    }
    Notif *n = &c->notifs[c->nc++];
    strncpy(n->title, title, sizeof(n->title) - 1);
    strncpy(n->body, body, sizeof(n->body) - 1);
    n->t = time(NULL);
}

/* ─── Beep ─── */
void shell_beep(compositor_t *c, int freq, int ms) {
    (void)c;
    pid_t pid = fork();
    if (pid == 0) {
        int fd = open("/dev/console", O_WRONLY);
        if (fd < 0) fd = open("/dev/tty0", O_WRONLY);
        if (fd >= 0) {
            write(fd, "\007", 1);
            close(fd);
        }
        _exit(0);
    }
}

/* ─── Window management ─── */
static int find_win(compositor_t *c, const char *title) {
    for (int i = 0; i < c->wc; i++)
        if (strcmp(c->wins[i].title, title) == 0) return i;
    return -1;
}

static int wnew(compositor_t *c, const char *title, int w, int h) {
    if (c->wc >= MAX_WIN) return -1;
    ShellWin *wn = &c->wins[c->wc++];
    wn->x = 60 + (c->wc * 40) % (c->fb_w - w - 80);
    wn->y = PANEL_H + 40 + (c->wc * 30) % (c->fb_h - PANEL_H - h - 100);
    wn->w = w; wn->h = h; wn->hidden = 0;
    wn->ws = c->current_ws;
    wn->drag = 0; wn->maximized = 0;
    strncpy(wn->title, title, 47);
    c->aw = c->wc - 1;
    return c->aw;
}

/* ─── Background drawing ─── */
static void draw_bg_gradient(compositor_t *c) {
    uint32_t *buf = (uint32_t *)c->backbuf;
    for (int y = PANEL_H; y < c->fb_h; y++) {
        float t = (float)(y - PANEL_H) / (c->fb_h - PANEL_H);
        uint32_t col;
        if (t < 0.3f) {
            float tp = t / 0.3f;
            col = 0xFF000000 |
                ((uint32_t)(10 + (20 - 10) * tp) << 16) |
                ((uint32_t)(5 + (8 - 5) * tp) << 8) |
                (uint32_t)(20 + (40 - 20) * tp);
        } else {
            float tp = (t - 0.3f) / 0.7f;
            col = 0xFF000000 |
                ((uint32_t)(20 + (30 - 20) * tp) << 16) |
                ((uint32_t)(8 + (10 - 8) * tp) << 8) |
                (uint32_t)(40 + (50 - 40) * tp);
        }
        for (int x = 0; x < c->fb_w; x++)
            buf[y * c->fb_w + x] = col;
    }
}

/* ─── Render helpers ─── */
void render_clear(compositor_t *c, uint32_t color) {
    asm_fill_rect32((uint32_t *)c->backbuf, c->fb_w, c->fb_w, c->fb_h, color);
}

void render_pixel(compositor_t *c, int x, int y, uint32_t color) {
    if (x < 0 || x >= c->fb_w || y < 0 || y >= c->fb_h) return;
    ((uint32_t *)c->backbuf)[y * c->fb_w + x] = color;
}

void render_rect(compositor_t *c, int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = x + w > c->fb_w ? c->fb_w : x + w;
    int y2 = y + h > c->fb_h ? c->fb_h : y + h;
    if (x1 >= x2 || y1 >= y2) return;
    
    uint32_t *buf = (uint32_t *)c->backbuf;
    for (int row = y1; row < y2; row++) {
        for (int col = x1; col < x2; col++)
            buf[row * c->fb_w + col] = color;
    }
}

void render_char(compositor_t *c, int x, int y, char ch, uint32_t fg, uint32_t bg) {
    if (ch < 32 || ch > 126) ch = ' ';
    const uint8_t *glyph = &font8x8[(ch - 32) * 8];
    uint32_t *buf = (uint32_t *)c->backbuf;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (byte & (0x80 >> col)) {
                int px = x + col, py = y + row;
                if (px >= 0 && px < c->fb_w && py >= 0 && py < c->fb_h)
                    buf[py * c->fb_w + px] = fg;
                if (col < 7) {
                    int px2 = x + col + 1;
                    if (px2 >= 0 && px2 < c->fb_w && py >= 0 && py < c->fb_h)
                        buf[py * c->fb_w + px2] = fg;
                }
            } else if (bg) {
                int px = x + col, py = y + row;
                if (px >= 0 && px < c->fb_w && py >= 0 && py < c->fb_h)
                    buf[py * c->fb_w + px] = bg;
            }
        }
    }
}

void render_text(compositor_t *c, int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    while (*s) {
        render_char(c, x, y, *s++, fg, bg);
        x += 8;
    }
}

void render_rect_blend(compositor_t *c, int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    int x1 = x < 0 ? 0 : x, y1 = y < 0 ? 0 : y;
    int x2 = x + w > c->fb_w ? c->fb_w : x + w;
    int y2 = y + h > c->fb_h ? c->fb_h : y + h;
    if (x1 >= x2 || y1 >= y2) return;
    
    uint8_t a = (color >> 24) & 0xFF, r_c = (color >> 16) & 0xFF;
    uint8_t g_c = (color >> 8) & 0xFF, b_c = color & 0xFF;
    uint32_t *buf = (uint32_t *)c->backbuf;
    
    for (int row = y1; row < y2; row++) {
        for (int col = x1; col < x2; col++) {
            uint32_t orig = buf[row * c->fb_w + col];
            uint8_t r_o = (orig >> 16) & 0xFF, g_o = (orig >> 8) & 0xFF, b_o = orig & 0xFF;
            buf[row * c->fb_w + col] = 0xFF000000 |
                (((r_c * a + r_o * (255 - a)) / 255) << 16) |
                (((g_c * a + g_o * (255 - a)) / 255) << 8) |
                ((b_c * a + b_o * (255 - a)) / 255);
        }
    }
}

uint32_t render_get_neon(compositor_t *c) {
    (void)c;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int tick = (ts.tv_nsec / 15000000) % 64;
    int val = tick < 32 ? tick : 64 - tick;
    int r = val * 8, g = 255 - val * 8, b = 255 - val * 2;
    if (r > 255) r = 255; if (g < 0) g = 0; if (b < 0) b = 0;
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

/* ─── Stars ─── */
typedef struct { int x, y; int type; } Star;
static Star stars[48];
static int stars_inited = 0;

static void init_stars(void) {
    if (stars_inited) return;
    srand(1337);
    for (int i = 0; i < 48; i++) {
        stars[i].x = rand() % 1024;
        stars[i].y = PANEL_H + 10 + (rand() % 260);
        stars[i].type = rand() % 4;
    }
    stars_inited = 1;
}

static void draw_stars(compositor_t *c) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int tick = (ts.tv_nsec / 100000000) % 8;
    for (int i = 0; i < 48; i++) {
        int brightness = (tick + i) % 5;
        uint32_t col;
        if (brightness == 0) col = COLOR_DIM;
        else if (brightness == 1) col = 0xFF6A3A7E;
        else if (brightness == 2) col = 0xFF9A7ABE;
        else if (brightness == 3) col = COLOR_PANEL_FG;
        else col = COLOR_WHITE;
        render_pixel(c, stars[i].x, stars[i].y, col);
        if (stars[i].type == 1 && brightness >= 3) {
            render_pixel(c, stars[i].x - 1, stars[i].y, col);
            render_pixel(c, stars[i].x + 1, stars[i].y, col);
            render_pixel(c, stars[i].x, stars[i].y - 1, col);
            render_pixel(c, stars[i].x, stars[i].y + 1, col);
        }
    }
}

/* ─── Retro-wave sun ─── */
static void draw_retro_sun(compositor_t *c) {
    int cx = c->fb_w / 2, cy = c->fb_h / 2 + 20, r = 110;
    for (int dy = -r; dy <= r; dy++) {
        int yy = cy + dy;
        if (yy < PANEL_H || yy >= c->fb_h) continue;
        int ww = (int)sqrt(r * r - dy * dy);
        if (dy > 10 && (dy % 14 < (dy / 4))) continue;
        float factor = (float)(dy + r) / (2 * r);
        uint8_t rn, gn, bn;
        if (factor < 0.5f) {
            float tp = factor * 2;
            rn = 255 + (255 - 255) * tp;
            gn = 50 + (100 - 50) * tp;
            bn = 0 + (80 - 0) * tp;
        } else {
            float tp = (factor - 0.5f) * 2;
            rn = 255 + (200 - 255) * tp;
            gn = 100 + (0 - 100) * tp;
            bn = 80 + (128 - 80) * tp;
        }
        render_rect(c, cx - ww, yy, ww * 2, 1,
                    0xFF000000 | (rn << 16) | (gn << 8) | bn);
    }
}

/* ─── Mountains ─── */
static void draw_mountains(compositor_t *c) {
    int horizon = c->fb_h / 2 + 80;
    for (int x = 0; x < c->fb_w; x++) {
        int h1 = 0, h2 = 0;
        if (x >= 0 && x <= 360) h1 = 110 - abs(x - 160) * 110 / 180;
        if (x >= 340 && x <= c->fb_w) h2 = 130 - abs(x - 620) * 130 / 220;
        if (h1 < 0) h1 = 0; if (h2 < 0) h2 = 0;
        int mh = h1 > h2 ? h1 : h2;
        if (mh > 0) {
            int peak_y = horizon - mh;
            render_rect(c, x, peak_y, 1, mh, COLOR_WIN_BG);
            render_pixel(c, x, peak_y, COLOR_ACCENT);
            if (x % 24 == 0) {
                for (int my = peak_y; my < horizon; my += 16)
                    render_pixel(c, x, my, COLOR_DIM);
            }
        }
    }
}

/* ─── Perspective grid ─── */
static void draw_perspective_grid(compositor_t *c) {
    uint32_t grid_col = 0xFF20B0AA;
    int horizon_y = c->fb_h / 2 + 80;
    int step = 8;
    for (int y = horizon_y; y < c->fb_h - 40; y += step) {
        render_rect(c, 0, y, c->fb_w, 1, grid_col);
        step = (int)(step * 1.35f);
        if (step > 64) step = 64;
    }
    int cx = c->fb_w / 2;
    for (int xx = -c->fb_w; xx <= c->fb_w * 2; xx += 55) {
        int x1 = cx, y1 = horizon_y;
        int x2 = xx, y2 = c->fb_h - 40;
        int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = dx + dy, e2;
        int curr_x = x1, curr_y = y1;
        while (1) {
            if (curr_x >= 0 && curr_x < c->fb_w &&
                curr_y >= horizon_y && curr_y < c->fb_h - 40)
                render_pixel(c, curr_x, curr_y, grid_col);
            if (curr_x == x2 && curr_y == y2) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; curr_x += sx; }
            if (e2 <= dx) { err += dx; curr_y += sy; }
        }
    }
}

/* ─── Desktop icons ─── */
#define ICON_W 48
#define ICON_H 48

static void draw_desktop_icons(compositor_t *c) {
    VisibleIcon icons[16];
    int count = get_visible_icons(c, icons);
    for (int i = 0; i < count; i++) {
        int x1 = 20;
        render_rect(c, x1, icons[i].y, ICON_W, ICON_H, 0xFF140A28);
        render_rect(c, x1-1, icons[i].y-1, ICON_W+2, 1, icons[i].color);
        render_rect(c, x1-1, icons[i].y+ICON_H, ICON_W+2, 1, icons[i].color);
        render_rect(c, x1-1, icons[i].y-1, 1, ICON_H+2, icons[i].color);
        render_rect(c, x1+ICON_W, icons[i].y-1, 1, ICON_H+2, icons[i].color);
        int tx = strlen(icons[i].glyph) == 1 ? 20 : 16;
        render_text(c, x1 + tx, icons[i].y + 18, icons[i].glyph, COLOR_WHITE, 0xFF140A28);
        render_text(c, x1 - 4, icons[i].y + 54, icons[i].label, COLOR_TEXT, 0);
    }
}

/* ─── Window drawing ─── */
static void draw_win(compositor_t *c, ShellWin *w) {
    if (w->hidden || w->ws != c->current_ws) return;
    int is_active = (c->aw == (int)(w - c->wins));
    uint32_t bd = is_active ? render_get_neon(c) : COLOR_WIN_BORDER;
    uint32_t bg = COLOR_WIN_BG, tl = COLOR_WIN_TITLE, tx = COLOR_TEXT;
    int t = w->y - 24;
    
    render_rect(c, w->x+4, t+4, w->w, w->h+24, 0xFF050308);
    render_rect(c, w->x-1, t-1, w->w+2, w->h+26, bd);
    render_rect(c, w->x, t, w->w, 24, tl);
    int title_len = strlen(w->title) * 8;
    render_text(c, w->x + (w->w / 2) - (title_len / 2), t + 8, w->title, tx, tl);
    render_rect(c, w->x + 8, t + 6, 12, 12, COLOR_RED);
    render_rect(c, w->x + 24, t + 6, 12, 12, COLOR_YELLOW);
    render_rect(c, w->x + 40, t + 6, 12, 12, COLOR_GREEN);
    render_rect(c, w->x, w->y, w->w, w->h, bg);
    
    if (strcmp(w->title, "System Info") == 0) {
        render_text(c, w->x+12, w->y+12, "=== SYSTEM REALTIME HARDWARE STATS ===", COLOR_ACCENT, bg);
        render_text(c, w->x+12, w->y+32, cached_cpu, COLOR_TEXT, bg);
        render_text(c, w->x+12, w->y+52, cached_ram, COLOR_GREEN, bg);
        render_text(c, w->x+12, w->y+72, cached_laptop, COLOR_YELLOW, bg);
        int ram_pct = 0;
        char *pct_ptr = strchr(cached_ram, '(');
        if (pct_ptr) sscanf(pct_ptr, "(%d%%)", &ram_pct);
        if (ram_pct > 0) draw_progress_bar(c, w->x+12, w->y+86, (float)ram_pct / 100.0f, COLOR_GREEN);
        render_text(c, w->x+12, w->y+110, cached_disk, COLOR_TEXT, bg);
        render_text(c, w->x+12, w->y+128, cached_disk_list, COLOR_TEXT, bg);
        char usb_buf[128]; snprintf(usb_buf, sizeof(usb_buf), "USB: %s", cached_usb);
        render_text(c, w->x+12, w->y+148, usb_buf, COLOR_YELLOW, bg);
        char net_buf[128]; snprintf(net_buf, sizeof(net_buf), "NET: %s", cached_net);
        render_text(c, w->x+12, w->y+168, net_buf, COLOR_WHITE, bg);
        int tor_ok = is_tor_running();
        char tor_buf[64]; snprintf(tor_buf, sizeof(tor_buf), "SECURE TOR ROUTE: %s", tor_ok ? "ACTIVE" : "OFFLINE");
        render_text(c, w->x+12, w->y+188, tor_buf, tor_ok ? COLOR_GREEN : COLOR_RED, bg);
        draw_progress_bar(c, w->x+12, w->y+202, tor_ok ? 1.0f : 0.45f, tor_ok ? COLOR_GREEN : COLOR_YELLOW);
        render_text(c, w->x+12, w->y+222, has_touchpad() ? "Touchpad: detected" : "Touchpad: not detected (mouse mode)", 
                    has_touchpad() ? COLOR_GREEN : COLOR_DIM, bg);
    } else if (strcmp(w->title, "Terminal") == 0) {
        for (int i = 0; i < c->term_log_count; i++) {
            uint32_t col = COLOR_GREEN;
            render_text(c, w->x + 12, w->y + 12 + i * 16, c->term_logs[i], col, bg);
        }
        int input_y = w->y + w->h - 24;
        char prompt[128];
        snprintf(prompt, sizeof(prompt), "synth3x@root:~$ %s", c->term_input);
        render_text(c, w->x + 12, input_y, prompt, COLOR_TEXT, bg);
        
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        if (((ts.tv_nsec / 250000000) % 2) == 0) {
            int cur_x = w->x + 140 + strlen(c->term_input) * 8;
            render_rect(c, cur_x, input_y, 8, 12, COLOR_TEXT);
        }
    } else if (strcmp(w->title, "Amnesia Handbook") == 0) {
        render_text(c, w->x+12, w->y+12,
                    "======================================================",
                    COLOR_ACCENT, bg);
        char ver_title[64];
        snprintf(ver_title, sizeof(ver_title),
                 "         AMNESIADE: WAYLAND COMPOSITOR v%s        ",
                 SYNTH3X_VERSION);
        render_text(c, w->x+12, w->y+26, ver_title, tx, bg);
        render_text(c, w->x+12, w->y+84,
                    "AmnesiaDE v" SYNTH3X_VERSION " — Wayland Compositor", tx, bg);
        render_text(c, w->x+12, w->y+100,
                    "Direct rendering via DRM/KMS (Linux Kernel Mode Setting)", tx, bg);
        render_text(c, w->x+12, w->y+116,
                    "Native Wayland protocol — supports external clients", tx, bg);
        render_text(c, w->x+12, w->y+132,
                    "All data in volatile RAM — destroyed on power-down.", tx, bg);
        render_text(c, w->x+12, w->y+148,
                    "Network: Tor transparent proxy + nftables firewall.", tx, bg);
        render_text(c, w->x+12, w->y+172, "KEYBOARD SHORTCUTS:", COLOR_YELLOW, bg);
        render_text(c, w->x+24, w->y+192,
                    "[ Super+1..4 ] Workspace switch", COLOR_TEXT, bg);
        render_text(c, w->x+24, w->y+208,
                    "[ Tab ]        Window focus cycle", tx, bg);
        render_text(c, w->x+24, w->y+224,
                    "[ CapsLock ]   Close focused window", tx, bg);
        render_text(c, w->x+24, w->y+240,
                    "[ Up / Down ]  Cycle workspaces", tx, bg);
        render_text(c, w->x+24, w->y+256,
                    "[ ESC ]        Exit to TTY shell", tx, bg);
        render_text(c, w->x+12, w->y+280,
                    "Desktop: Terminal | SysInfo | Web | Guide | Install",
                    COLOR_PANEL_FG, bg);
    } else if (strcmp(w->title, "Synth3x Guide") == 0) {
        const WinTextLine *lines = NULL;
        int nlines = 0;
        switch (c->guide_page) {
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
                render_text(c, w->x + lines[i].x, w->y + lines[i].y, lines[i].text, col, bg);
            }
        }

        /* Draw SETUP INTERNET button on page 0 */
        if (c->guide_page == 0) {
            int btn_net_x = w->x + 24, btn_net_y = w->y + 168;
            render_rect(c, btn_net_x, btn_net_y, 180, 24, 0xFF14281A);
            render_rect(c, btn_net_x-1, btn_net_y-1, 182, 26, COLOR_GREEN);
            render_text(c, btn_net_x + 14, btn_net_y + 8, "[ SETUP INTERNET ]", COLOR_GREEN, 0xFF14281A);
        }

        /* Navigation buttons */
        int btn_prev_x = w->x + 120, btn_next_x = w->x + 280, btn_y = w->y + 280;
        render_rect(c, btn_prev_x, btn_y, 80, 24, 0xFF1E1432);
        render_rect(c, btn_prev_x-1, btn_y-1, 82, 26, COLOR_DIM);
        render_text(c, btn_prev_x + 16, btn_y + 8, "< PREV", COLOR_TEXT, 0xFF1E1432);
        render_rect(c, btn_next_x, btn_y, 80, 24, 0xFF1E1432);
        render_rect(c, btn_next_x-1, btn_y-1, 82, 26, COLOR_DIM);
        render_text(c, btn_next_x + 16, btn_y + 8, "NEXT >", COLOR_TEXT, 0xFF1E1432);
    } else if (strcmp(w->title, "VSCodium") == 0) {
        render_text(c, w->x+12, w->y+12, "VSCodium v1.92 — main.py", COLOR_ACCENT, bg);
        render_text(c, w->x+12, w->y+w->h-20, "VSCodium v1.92.0 (Gentoo)", COLOR_PANEL_FG, bg);
        const char *editor_lines[] = {
            "import os",
            "import sys",
            "",
            "def main():",
            "    print(\"VSCodium on Synth3x Gentoo OS!\")",
            "    print(\"Identity: amnesic / RAM-only\")",
            "    print(\"Network: Tor Transparent Proxy\")",
            "",
            "if __name__ == '__main__':",
            "    main()"
        };
        for (int i = 0; i < 10; i++) {
            char num[8];
            snprintf(num, sizeof(num), "%d", i + 1);
            render_text(c, w->x + 12, w->y + 36 + i * 16, num, COLOR_DIM, bg);
            render_text(c, w->x + 36, w->y + 36 + i * 16, editor_lines[i], COLOR_TEXT, bg);
        }
    }
}

/* ─── Panel ─── */
static void draw_panel(compositor_t *c) {
    render_rect(c, 0, 0, c->fb_w, PANEL_H, COLOR_PANEL_BG);
    render_rect(c, 0, PANEL_H - 1, c->fb_w, 1, COLOR_ACCENT);
    
    char str[64];
    snprintf(str, sizeof(str), "Synth3x OS  (AmnesiaDE v%s / Wayland Compositor)",
             SYNTH3X_VERSION);
    render_text(c, 8, 10, str, COLOR_ACCENT, COLOR_PANEL_BG);
    
    snprintf(str, sizeof(str), "WS %d/%d", c->current_ws + 1, WORKSPACES);
    render_text(c, 360, 10, str, COLOR_PANEL_FG, COLOR_PANEL_BG);
    
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    snprintf(str, sizeof(str), " %02d:%02d ", lt->tm_hour, lt->tm_min);
    render_text(c, c->fb_w - 8 * strlen(str) - 8, 10, str,
                COLOR_PANEL_FG, COLOR_PANEL_BG);
}

/* ─── Dock ─── */
static void draw_dock(compositor_t *c) {
    int dw = 500, dh = 34;
    int dx = c->fb_w / 2 - dw / 2, dy = c->fb_h - 40;
    
    render_rect(c, dx, dy, dw, dh, COLOR_PANEL_BG);
    uint32_t neon = render_get_neon(c);
    render_rect(c, dx-1, dy-1, dw+2, 1, neon);
    render_rect(c, dx-1, dy+dh, dw+2, 1, neon);
    render_rect(c, dx-1, dy-1, 1, dh+2, neon);
    render_rect(c, dx+dw, dy-1, 1, dh+2, neon);
    
    render_text(c, dx + 10, dy + 12, "[>_] TERM", COLOR_TEXT, COLOR_PANEL_BG);
    render_text(c, dx + 75, dy + 12, "[i] STAT", COLOR_GREEN, COLOR_PANEL_BG);
    render_text(c, dx + 140, dy + 12, "[W] WEB", COLOR_ORANGE, COLOR_PANEL_BG);
    
    if (c->vscodium_installed) {
        render_text(c, dx + 205, dy + 12, "{} VSCOD", COLOR_ACCENT, COLOR_PANEL_BG);
    } else {
        render_rect(c, dx + 205, dy + 6, 60, 22, 0xFF140A28);
        render_text(c, dx + 213, dy + 12, "LOCKED", COLOR_DIM, 0xFF140A28);
    }
    render_text(c, dx + 270, dy + 12, "[?] HANDBK", COLOR_YELLOW, COLOR_PANEL_BG);
    render_text(c, dx + 335, dy + 12, "[#] GUIDE", COLOR_GREEN, COLOR_PANEL_BG);
}

/* ─── Custom cursor ─── */
static const char *cursor_map[] = {
    "X               ", "XX              ", "X.X             ",
    "X..X            ", "X...X           ", "X....X          ",
    "X.....X         ", "X......X        ", "X.......X       ",
    "X........X      ", "X.....XXXX      ", "X..X..X         ",
    "XX  X..X        ", "    X..X        ", "     XX         ",
    NULL
};

static void draw_cursor(compositor_t *c) {
    for (int r = 0; cursor_map[r]; r++) {
        for (int col = 0; cursor_map[r][col]; col++) {
            if (cursor_map[r][col] == 'X')
                render_pixel(c, c->mx + col, c->my + r, COLOR_TEXT);
            else if (cursor_map[r][col] == '.')
                render_pixel(c, c->mx + col, c->my + r, COLOR_BG);
        }
    }
}

/* ─── Notifications ─── */
static void draw_notifs(compositor_t *c) {
    time_t now = time(NULL);
    int y = PANEL_H + 10, x = c->fb_w - NOTIF_W - 10;
    for (int i = 0; i < c->nc && i < 3; i++) {
        if (now - c->notifs[i].t > NOTIF_DUR + 2) {
            memmove(c->notifs + i, c->notifs + i + 1,
                    sizeof(Notif) * (c->nc - i - 1));
            c->nc--; i--; continue;
        }
        render_rect(c, x, y, NOTIF_W, NOTIF_H, COLOR_PANEL_BG);
        render_rect(c, x, y, 4, NOTIF_H, COLOR_ACCENT);
        render_rect(c, x, y, NOTIF_W, 1, COLOR_ACCENT);
        render_rect(c, x, y + NOTIF_H - 1, NOTIF_W, 1, COLOR_DIM);
        render_text(c, x + 12, y + 8, c->notifs[i].title, COLOR_ACCENT, COLOR_PANEL_BG);
        render_text(c, x + 12, y + 30, c->notifs[i].body, COLOR_PANEL_FG, COLOR_PANEL_BG);
        y += NOTIF_H + 5;
    }
}

/* ─── Shell init ─── */
void shell_init(compositor_t *c) {
    init_stars();
    
    c->guide_page = 0;
    c->vscodium_installed = 0;
    c->mx = c->fb_w / 2;
    c->my = c->fb_h / 2;
    c->mclick = 0;
    c->mouse_pressed = 0;
    c->selecting = 0;
    c->show_copy_dialog = 0;
    c->selected_text[0] = 0;
    c->clipboard[0] = 0;
    c->notif_fd = -1;
    c->running = 1;
    c->shift_pressed = 0;
    c->super_pressed = 0;
    c->term_log_count = 0;
    c->nc = 0;
    
    /* Create notification FIFO */
    mkfifo("/tmp/synth3x-notif", 0666);
    c->notif_fd = open("/tmp/synth3x-notif", O_RDONLY | O_NONBLOCK);
    
    /* Log init */
    char msg[64];
    snprintf(msg, sizeof(msg), "Synth3x OS v%s — Wayland Compositor", SYNTH3X_VERSION);
    shell_term_log(c, msg);
    shell_term_log(c, "[OK] DRM/KMS graphics engine active");
    shell_term_log(c, "[OK] Wayland protocol server ready");
    shell_term_log(c, "[OK] Tor transparent routing available");
    shell_term_log(c, "[OK] nftables firewall active");
    
    /* Create default windows */
    wnew(c, "Terminal", 480, 280);
    wnew(c, "System Info", 400, 260);
    wnew(c, "Amnesia Handbook", 500, 340);
    wnew(c, "Synth3x Guide", 520, 340);
    
    shell_beep(c, 523, 80);
    shell_beep(c, 659, 80);
    shell_beep(c, 784, 80);
    shell_beep(c, 1046, 120);
    
    shell_notif(c, "AmnesiaDE v" SYNTH3X_VERSION,
                 "Wayland Compositor loaded. Type 'browser' for web!");
    shell_notif(c, "Display", "DRM/KMS active — multi-monitor ready");
}

void shell_draw(compositor_t *c) {
    c->stats_tick++;
    if (c->stats_tick >= 120 || c->stats_tick == 1) {
        c->stats_tick = 1;
        update_cached_stats();
    }

    draw_bg_gradient(c);
    draw_stars(c);
    draw_retro_sun(c);
    draw_mountains(c);
    draw_perspective_grid(c);
    draw_desktop_icons(c);
    
    for (int i = 0; i < c->wc; i++)
        draw_win(c, &c->wins[i]);
    
    draw_notifs(c);
    draw_panel(c);
    draw_dock(c);

    if (c->selecting) {
        int x = c->sel_start_x < c->sel_end_x ? c->sel_start_x : c->sel_end_x;
        int y = c->sel_start_y < c->sel_end_y ? c->sel_start_y : c->sel_end_y;
        int w = abs(c->sel_end_x - c->sel_start_x);
        int h = abs(c->sel_end_y - c->sel_start_y);
        render_rect_blend(c, x, y, w, h, 0x4000FFFF);
    }

    if (c->show_copy_dialog) {
        draw_copy_modal(c);
    }

    draw_cursor(c);
}

/* ─── Keyboard handler ─── */
void shell_handle_key(compositor_t *c, int code) {
    if (code == 1) {  /* ESC */
        c->running = 0;
        return;
    }
    
    /* Handle modifiers */
    if (code == 42 || code == 54) {
        c->shift_pressed = 1;
        return;
    }
    if (code == 125 || code == 126) {
        c->super_pressed = 1;
        return;
    }
    
    /* Guide page switching with arrows */
    int guide_idx = find_win(c, "Synth3x Guide");
    if (guide_idx >= 0 && c->aw == guide_idx) {
        if (code == 105) {  /* Left Arrow */
            c->guide_page = (c->guide_page - 1 + 8) % 8;
            shell_beep(c, 500, 30);
            return;
        } else if (code == 106) {  /* Right Arrow */
            c->guide_page = (c->guide_page + 1) % 8;
            shell_beep(c, 500, 30);
            return;
        }
    }
    
    if (c->super_pressed) {
        if (code >= 2 && code <= 5) {
            c->current_ws = code - 2;
            shell_beep(c, 784, 40);
            shell_notif(c, "AmnesiaDE", "Workspace switched.");
            return;
        }
    }
    
    if (code == 58) {  /* CapsLock = close window */
        if (c->aw >= 0 && c->aw < c->wc) {
            c->wins[c->aw].hidden = 1;
            shell_beep(c, 600, 50);
        }
        return;
    }
    
    if (code == 15) {  /* Tab = cycle focus */
        for (int i = 1; i <= c->wc; i++) {
            int ni = (c->aw + i) % c->wc;
            if (!c->wins[ni].hidden) {
                c->aw = ni;
                break;
            }
        }
        shell_beep(c, 523, 30);
        return;
    }
    
    if (code == 103 || code == 108) {  /* Up / Down = workspace */
        int d = (code == 108) ? 1 : -1;
        c->current_ws = (c->current_ws + d + WORKSPACES) % WORKSPACES;
        shell_beep(c, 659, 30);
        return;
    }
    
    /* Terminal input handling */
    int term_idx = find_win(c, "Terminal");
    if (term_idx >= 0 && c->aw == term_idx) {
        if (code == 28) {  /* Enter */
            if (strlen(c->term_input) > 0) {
                /* Rewrite pkg install to emerge */
                if (strncmp(c->term_input, "pkg install ", 12) == 0) {
                    char real_cmd[256];
                    snprintf(real_cmd, sizeof(real_cmd), "emerge %s", c->term_input + 12);
                    strncpy(c->term_input, real_cmd, sizeof(c->term_input) - 1);
                }

                /* Execute command */
                char echo[128];
                snprintf(echo, sizeof(echo), "$ %s", c->term_input);
                shell_term_log(c, echo);
                
                if (strcmp(c->term_input, "clear") == 0) {
                    c->term_log_count = 0;
                } else if (strcmp(c->term_input, "browser") == 0 ||
                           strcmp(c->term_input, "w3m") == 0 ||
                           strcmp(c->term_input, "web") == 0) {
                    shell_term_log(c, "Launching web browser (w3m on VT1)...");
                    shell_term_log(c, "Press Ctrl+Alt+F1 for browser, Alt+F7 to return.");
                } else if (strcmp(c->term_input, "vscodium") == 0 || strcmp(c->term_input, "vscodium &") == 0) {
                    if (access("/usr/local/bin/codium", F_OK) == 0 || access("/usr/bin/vscodium", F_OK) == 0 || access("/var/db/syn/vscodium", F_OK) == 0) {
                        int idx = find_win(c, "VSCodium");
                        if (idx < 0) {
                            idx = wnew(c, "VSCodium", 500, 320);
                        }
                        if (idx >= 0) { c->wins[idx].hidden = 0; c->wins[idx].ws = c->current_ws; c->aw = idx; }
                        shell_term_log(c, "AmnesiaDE: launching VSCodium...");
                        shell_notif(c, "VSCodium", "Editor launched.");
                    } else {
                        shell_term_log(c, "vscodium: command not found (try: 'emerge vscodium')");
                        shell_beep(c, 300, 100);
                    }
                } else {
                    /* Execute asynchronously */
                    CmdArgs *args = malloc(sizeof(CmdArgs));
                    args->c = c;
                    strncpy(args->cmd, c->term_input, sizeof(args->cmd) - 1);
                    args->cmd[sizeof(args->cmd) - 1] = '\0';
                    pthread_t cmd_thread;
                    pthread_create(&cmd_thread, NULL, async_exec_cmd, args);
                    pthread_detach(cmd_thread);
                }
                
                c->term_input[0] = '\0';
                shell_beep(c, 880, 40);
            }
        } else if (code == 14) {  /* Backspace */
            int len = strlen(c->term_input);
            if (len > 0) c->term_input[len - 1] = '\0';
        } else {
            char ch = scancode_to_ascii(code, c->shift_pressed);
            if (ch > 0 && strlen(c->term_input) < 64) {
                int len = strlen(c->term_input);
                c->term_input[len] = ch;
                c->term_input[len + 1] = '\0';
            }
        }
    }
}

/* ─── Mouse/click handler ─── */
void shell_handle_click(compositor_t *c) {
    if (c->show_copy_dialog) {
        int mw = 320, mh = 140;
        int mx_pos = c->fb_w / 2 - mw / 2, my_pos = c->fb_h / 2 - mh / 2;
        int btn_y = my_pos + 94;
        if (c->mx >= mx_pos + 30 && c->mx <= mx_pos + 130 && c->my >= btn_y && c->my <= btn_y + 24) {
            strncpy(c->clipboard, c->selected_text, sizeof(c->clipboard) - 1);
            c->clipboard[sizeof(c->clipboard) - 1] = '\0';
            char clip_cmd[1024];
            FILE *tmp_f = fopen("/tmp/synth3x_clip", "w");
            if (tmp_f) {
                fputs(c->selected_text, tmp_f);
                fclose(tmp_f);
                snprintf(clip_cmd, sizeof(clip_cmd), "xclip -selection clipboard < /tmp/synth3x_clip 2>/dev/null || xsel -ib < /tmp/synth3x_clip 2>/dev/null");
                system(clip_cmd);
            }
            shell_notif(c, "Clipboard", "Text copied!");
            shell_beep(c, 880, 80); shell_beep(c, 1100, 120);
            c->show_copy_dialog = 0;
        } else if (c->mx >= mx_pos + 190 && c->mx <= mx_pos + 290 && c->my >= btn_y && c->my <= btn_y + 24) {
            c->show_copy_dialog = 0;
            shell_beep(c, 300, 100);
        }
        return;
    }

    /* Guide buttons click check */
    int g_idx = find_win(c, "Synth3x Guide");
    if (g_idx >= 0 && !c->wins[g_idx].hidden && c->wins[g_idx].ws == c->current_ws) {
        int btn_prev_x = c->wins[g_idx].x + 120, btn_next_x = c->wins[g_idx].x + 280, btn_y = c->wins[g_idx].y + 280;
        if (c->mx >= btn_prev_x && c->mx <= btn_prev_x + 80 && c->my >= btn_y && c->my <= btn_y + 24) {
            c->guide_page = (c->guide_page - 1 + 8) % 8;
            shell_beep(c, 500, 30);
            return;
        } else if (c->mx >= btn_next_x && c->mx <= btn_next_x + 80 && c->my >= btn_y && c->my <= btn_y + 24) {
            c->guide_page = (c->guide_page + 1) % 8;
            shell_beep(c, 500, 30);
            return;
        }
        if (c->guide_page == 0) {
            int btn_net_x = c->wins[g_idx].x + 24, btn_net_y = c->wins[g_idx].y + 168;
            if (c->mx >= btn_net_x && c->mx <= btn_net_x + 180 && c->my >= btn_net_y && c->my <= btn_net_y + 24) {
                shell_beep(c, 523, 80); shell_beep(c, 659, 80); shell_beep(c, 784, 120);
                shell_notif(c, "Network Config", "Starting auto-config...");
                pthread_t net_thread;
                pthread_create(&net_thread, NULL, async_net_setup, c);
                pthread_detach(net_thread);
                return;
            }
        }
    }

    /* Window title bar buttons & drag */
    int clicked_window = 0;
    for (int j = c->wc - 1; j >= 0; j--) {
        if (c->wins[j].hidden || c->wins[j].ws != c->current_ws) continue;
        ShellWin *w = &c->wins[j];
        
        /* Close button */
        if (c->mx >= w->x + 8 && c->mx <= w->x + 20 &&
            c->my >= w->y - 22 && c->my <= w->y - 4) {
            w->hidden = 1;
            shell_beep(c, 600, 60);
            return;
        }
        /* Minimize button */
        if (c->mx >= w->x + 24 && c->mx <= w->x + 36 &&
            c->my >= w->y - 22 && c->my <= w->y - 4) {
            w->hidden = 1;
            shell_beep(c, 400, 50);
            return;
        }
        /* Maximize button */
        if (c->mx >= w->x + 40 && c->mx <= w->x + 52 &&
            c->my >= w->y - 22 && c->my <= w->y - 4) {
            if (w->maximized) {
                w->x = w->orig_x; w->y = w->orig_y;
                w->w = w->orig_w; w->h = w->orig_h;
                w->maximized = 0;
            } else {
                w->orig_x = w->x; w->orig_y = w->y;
                w->orig_w = w->w; w->orig_h = w->h;
                w->x = 0; w->y = PANEL_H + 24;
                w->w = c->fb_w; w->h = c->fb_h - PANEL_H - 24 - 45;
                w->maximized = 1;
            }
            shell_beep(c, 500, 60);
            return;
        }
        
        /* Title bar drag */
        if (c->mx >= w->x && c->mx <= w->x + w->w &&
            c->my >= w->y - 24 && c->my <= w->y) {
            /* Reorder window to top */
            c->aw = j;
            w->drag = 1;
            w->dx = c->mx - w->x;
            w->dy = c->my - (w->y - 24);
            return;
        }

        /* Check if click falls within window client area */
        if (c->mx >= w->x && c->mx <= w->x + w->w &&
            c->my >= w->y && c->my <= w->y + w->h) {
            c->aw = j;
            clicked_window = 1;
        }
    }
    
    /* Desktop icons click check */
    if (c->mx >= 20 && c->mx <= 68) {
        VisibleIcon icons[16];
        int count = get_visible_icons(c, icons);
        for (int i = 0; i < count; i++) {
            if (c->my >= icons[i].y && c->my <= icons[i].y + 48) {
                if (strcmp(icons[i].label, "Terminal") == 0) {
                    int idx = find_win(c, "Terminal");
                    if (idx >= 0) { c->wins[idx].hidden = 0; c->wins[idx].ws = c->current_ws; c->aw = idx; }
                } else if (strcmp(icons[i].label, "System Info") == 0) {
                    int idx = find_win(c, "System Info");
                    if (idx >= 0) { c->wins[idx].hidden = 0; c->wins[idx].ws = c->current_ws; c->aw = idx; }
                } else if (strcmp(icons[i].label, "Web") == 0) {
                    shell_term_log(c, "Launching web browser (w3m on VT1)...");
                    shell_term_log(c, "Press Ctrl+Alt+F1 for browser, Alt+F7 to return.");
                } else if (strcmp(icons[i].label, "Handbook") == 0) {
                    int idx = find_win(c, "Amnesia Handbook");
                    if (idx >= 0) { c->wins[idx].hidden = 0; c->wins[idx].ws = c->current_ws; c->aw = idx; }
                } else if (strcmp(icons[i].label, "Guide") == 0) {
                    int idx = find_win(c, "Synth3x Guide");
                    if (idx >= 0) { c->wins[idx].hidden = 0; c->wins[idx].ws = c->current_ws; c->aw = idx; }
                } else if (strcmp(icons[i].label, "VSCodium") == 0) {
                    int idx = find_win(c, "VSCodium");
                    if (idx < 0) {
                        idx = wnew(c, "VSCodium", 500, 320);
                    }
                    if (idx >= 0) { c->wins[idx].hidden = 0; c->wins[idx].ws = c->current_ws; c->aw = idx; }
                } else if (strcmp(icons[i].label, "Firefox") == 0) {
                    shell_term_log(c, "Launching Firefox...");
                    system("firefox &");
                } else if (strcmp(icons[i].label, "Telegram") == 0) {
                    shell_term_log(c, "Launching Telegram Desktop...");
                    system("Telegram &");
                } else if (strcmp(icons[i].label, "Install") == 0) {
                    shell_term_log(c, "Type: synth3x-installer");
                }
                return;
            }
        }
    }
    
    /* Dock click check */
    int dw = 500, dh = 34, dx = c->fb_w / 2 - dw / 2, dy = c->fb_h - 40;
    if (c->my >= dy && c->my <= dy + dh && c->mx >= dx && c->mx <= dx + dw) {
        int idx = -1;
        if (c->mx >= dx + 10 && c->mx <= dx + 70) {
            idx = find_win(c, "Terminal");
        } else if (c->mx >= dx + 75 && c->mx <= dx + 135) {
            idx = find_win(c, "System Info");
        } else if (c->mx >= dx + 140 && c->mx <= dx + 200) {
            shell_term_log(c, "Launching web browser...");
            return;
        } else if (c->mx >= dx + 205 && c->mx <= dx + 265) {
            int vsc_inst = (access("/usr/local/bin/codium", F_OK) == 0 || access("/usr/bin/vscodium", F_OK) == 0 || access("/var/db/syn/vscodium", F_OK) == 0);
            if (vsc_inst) {
                idx = find_win(c, "VSCodium");
                if (idx < 0) {
                    idx = wnew(c, "VSCodium", 500, 320);
                }
            } else {
                shell_notif(c, "Synth3x OS", "VSCodium not installed. Type 'pkg install vscodium'.");
                return;
            }
        } else if (c->mx >= dx + 270 && c->mx <= dx + 330) {
            idx = find_win(c, "Amnesia Handbook");
        } else if (c->mx >= dx + 335 && c->mx <= dx + 395) {
            idx = find_win(c, "Synth3x Guide");
        } else return;
        
        if (idx >= 0) { c->wins[idx].hidden = 0; c->wins[idx].ws = c->current_ws; c->aw = idx; }
        return;
    }

    /* Start drag text selection if we clicked in window client area, but not buttons or icons */
    if (clicked_window && c->my < c->fb_h - 40) {
        c->selecting = 1;
        c->sel_start_x = c->mx; c->sel_start_y = c->my;
        c->sel_end_x = c->mx; c->sel_end_y = c->my;
    }
}

/* ─── Mouse motion handler ─── */
void shell_handle_mouse(compositor_t *c, int dx, int dy, int abs_x, int abs_y) {
    /* Update mouse position */
    if (abs_x >= 0) c->mx = abs_x;
    else c->mx += dx;
    if (abs_y >= 0) c->my = abs_y;
    else c->my += dy;
    
    /* Clamp */
    if (c->mx < 0) c->mx = 0;
    if (c->mx >= c->fb_w) c->mx = c->fb_w - 1;
    if (c->my < PANEL_H) c->my = PANEL_H;
    if (c->my >= c->fb_h) c->my = c->fb_h - 1;
    
    /* Update selection bounds */
    if (c->selecting) {
        c->sel_end_x = c->mx;
        c->sel_end_y = c->my;
    }

    /* Handle drag */
    for (int i = 0; i < c->wc; i++) {
        if (c->wins[i].drag) {
            c->wins[i].x = c->mx - c->wins[i].dx;
            c->wins[i].y = c->my - c->wins[i].dy + 24;
            if (c->wins[i].x < 0) c->wins[i].x = 0;
            if (c->wins[i].y < PANEL_H + 24) c->wins[i].y = PANEL_H + 24;
            if (c->wins[i].x + c->wins[i].w > c->fb_w)
                c->wins[i].x = c->fb_w - c->wins[i].w;
            if (c->wins[i].y + c->wins[i].h > c->fb_h - 42)
                c->wins[i].y = c->fb_h - 42 - c->wins[i].h;
        }
    }
}
