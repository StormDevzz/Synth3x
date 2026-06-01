/* Synth3x OS — init (PID 1) — with hardware detection + Gentoo-style boot */

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
#define SHELL       "/bin/bash"

/* Hardware detection */
#include "../hardware/hw_detect.h"

/* VGA text buffer direct write (for diagnostics) */
static void vga_write(const char *s) {
    int fd = open("/dev/tty0", O_WRONLY);
    if (fd >= 0) { write(fd, s, strlen(s)); close(fd); }
    /* Also write to serial console for -nographic debugging */
    int sf = open("/dev/ttyS0", O_WRONLY);
    if (sf >= 0) { write(sf, s, strlen(s)); close(sf); }
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

#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <dirent.h>

static void randomize_mac(const char *ifname) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return;
    
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(s, SIOCGIFFLAGS, &ifr) >= 0) {
        ifr.ifr_flags &= ~IFF_UP;
        ioctl(s, SIOCSIFFLAGS, &ifr);
    }
    
    srand(time(NULL) ^ getpid());
    ifr.ifr_hwaddr.sa_family = 1;
    ifr.ifr_hwaddr.sa_data[0] = 0x02;
    for (int i = 1; i < 6; i++) {
        ifr.ifr_hwaddr.sa_data[i] = rand() % 256;
    }
    
    ioctl(s, SIOCSIFHWADDR, &ifr);
    
    if (ioctl(s, SIOCGIFFLAGS, &ifr) >= 0) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
        ioctl(s, SIOCSIFFLAGS, &ifr);
    }
    
    close(s);
}

static void randomize_all_macs(void) {
    DIR *d = opendir("/sys/class/net");
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d))) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, "lo") == 0)
            continue;
        randomize_mac(de->d_name);
    }
    closedir(d);
}

static void randomize_hostname(void) {
    char hostname[32];
    srand(time(NULL) ^ getpid());
    snprintf(hostname, sizeof(hostname), "synth-%04x", rand() % 0xffff);
    sethostname(hostname, strlen(hostname));
}

static void bring_lo_up(void) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s >= 0) {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, "lo");
        if (ioctl(s, SIOCGIFFLAGS, &ifr) >= 0) {
            ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
            ioctl(s, SIOCSIFFLAGS, &ifr);
        }
        close(s);
    }
}

static void setup_resolv_conf(void) {
    int fd = open("/etc/resolv.conf", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        const char *conf = "nameserver 1.1.1.1\nnameserver 8.8.8.8\nnameserver 127.0.0.1\n";
        write(fd, conf, strlen(conf));
        close(fd);
    }
}

/* ─── Network interface detection & configuration ─── */
static void list_interfaces(void) {
    vga_write("init: available network interfaces:\n");
    DIR *d = opendir("/sys/class/net");
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d))) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        char path[256];
        snprintf(path, sizeof(path), "/sys/class/net/%s/address", de->d_name);
        int fd = open(path, O_RDONLY);
        char mac[32] = "??";
        if (fd >= 0) {
            int n = read(fd, mac, sizeof(mac)-1);
            if (n > 0) { mac[n] = '\0'; char *nl = strchr(mac, '\n'); if (nl) *nl = '\0'; }
            close(fd);
        }
        char buf[128];
        snprintf(buf, sizeof(buf), "  %s  (%s)\n", de->d_name, mac);
        vga_write(buf);
    }
    closedir(d);
}

static void run_dhcp(void) {
    DIR *d = opendir("/sys/class/net");
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d))) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, "lo") == 0)
            continue;
        /* Skip virtual interfaces */
        if (strstr(de->d_name, "docker") || strstr(de->d_name, "veth") ||
            strstr(de->d_name, "br-") || strstr(de->d_name, "tun"))
            continue;

        pid_t pid = fork();
        if (pid == 0) {
            /* Try dhcpcd first, fallback to udhcpc */
            execl("/sbin/dhcpcd", "dhcpcd", "-q", de->d_name, NULL);
            execl("/bin/busybox", "busybox", "udhcpc", "-i", de->d_name, "-b", "-q", "-f", NULL);
            _exit(1);
        }
    }
    closedir(d);
}

/* ─── Gentoo-style colorful boot messages ─── */
static void boot_msg(const char *module, const char *status, const char *color) {
    char buf[128];
    snprintf(buf, sizeof(buf), "  [ %s ]  %s\n", status, module);
    vga_write(buf);
}

/* ─── Main ─── */
int main(int argc, char *argv[]) {
    setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);

    /* ─── Gentoo-style boot sequence ─── */
    vga_write("\n");
    vga_write(" Synth3x-Anon OS  —  Gentoo Hardened Profile\n");
    vga_write("================================================================\n");
    vga_write(" * Mounting filesystems ...\n");

    /* Mount filesystems */
    mkdir("/proc", 0755); mount("proc", "/proc", "proc", 0, NULL);
    boot_msg("proc", "OK", "32");
    mkdir("/sys", 0755);  mount("sysfs", "/sys", "sysfs", 0, NULL);
    boot_msg("sysfs", "OK", "32");
    mkdir("/dev", 0755);  mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    boot_msg("devtmpfs", "OK", "32");
    mkdir("/tmp", 0755);  mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
    boot_msg("tmpfs", "OK", "32");

    /* ─── Hardware detection ─── */
    vga_write(" * Detecting hardware ...\n");

    /* CPU detection via assembly */
    char cpu_vendor[16] = "";
    hw_cpuid_vendor(cpu_vendor);
    char cpu_brand[64] = "";
    hw_cpuid_brand_string(cpu_brand);

    char hw_buf[128];
    snprintf(hw_buf, sizeof(hw_buf), "   CPU: %s  (%s)\n", cpu_vendor, cpu_brand);
    vga_write(hw_buf);

    /* DMI detection */
    char dmi_vendor[64] = "", dmi_product[64] = "";
    if (hw_dmi_vendor(dmi_vendor, sizeof(dmi_vendor)) == 0) {
        snprintf(hw_buf, sizeof(hw_buf), "   Vendor: %s\n", dmi_vendor);
        vga_write(hw_buf);
    }
    if (hw_dmi_product(dmi_product, sizeof(dmi_product)) == 0) {
        snprintf(hw_buf, sizeof(hw_buf), "   Model: %s\n", dmi_product);
        vga_write(hw_buf);
    }

    /* Touchpad detection */
    if (hw_has_touchpad()) {
        vga_write("   Touchpad: detected\n");
    } else {
        vga_write("   Touchpad: not detected (PS/2 mouse fallback)\n");
    }

    if (hw_is_laptop()) {
        vga_write("   Chassis: Laptop\n");
    } else {
        vga_write("   Chassis: Desktop\n");
    }

    /* ─── Auto-configure hardware (load modules) ─── */
    vga_write(" * Loading hardware modules ...\n");
    hw_auto_configure();
    boot_msg("hardware modules", "OK", "32");

    /* Setup users and groups for Tor security */
    mkdir("/etc", 0755);
    int pwd_fd = open("/etc/passwd", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (pwd_fd >= 0) {
        const char *pwd = "root:x:0:0:root:/root:/bin/sh\ntor:x:100:100:tor:/var/lib/tor:/bin/sh\n";
        write(pwd_fd, pwd, strlen(pwd));
        close(pwd_fd);
    }
    int grp_fd = open("/etc/group", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (grp_fd >= 0) {
        const char *grp = "root:x:0:\ntor:x:100:\n";
        write(grp_fd, grp, strlen(grp));
        close(grp_fd);
    }

    mkdir("/var", 0755);
    mkdir("/var/lib", 0755);
    mkdir("/var/lib/tor", 0700);
    mkdir("/var/log", 0755);
    mkdir("/var/log/tor", 0755);
    mkdir("/run", 0755);
    chown("/var/lib/tor", 100, 100);
    chown("/var/log/tor", 100, 100);

    vga_write(" * Randomizing network identity ...\n");
    randomize_hostname();
    randomize_all_macs();
    bring_lo_up();
    setup_resolv_conf();
    boot_msg("MAC spoofing", "OK", "32");
    boot_msg("hostname randomization", "OK", "32");

    /* ─── Networking ─── */
    vga_write(" * Starting firewall (nftables) ...\n");
    if (fork() == 0) {
        execl("/usr/sbin/nft", "nft", "-f", "/etc/nftables.rules", NULL);
        _exit(1);
    }
    sleep(1);
    boot_msg("nftables", "OK", "32");

    vga_write(" * Starting network interfaces ...\n");
    list_interfaces();
    run_dhcp();
    boot_msg("DHCP", "started", "33");

    /* Tor is lazy-started to save RAM (~30MB). Run 'tor-start' when needed. */
    vga_write(" * Tor: lazy (run 'tor-start' to enable)\n");

    /* ─── Load GPU + Input drivers ─── */
    vga_write(" * Loading graphics & input modules ...\n");
    {
        const char *mods[] = {
            "/lib/modules/ttm.ko",
            "/lib/modules/bochs.ko",
            "/lib/modules/virtio-gpu.ko",
            "/lib/modules/serio_raw.ko",
            "/lib/modules/psmouse.ko",
            "/lib/modules/mousedev.ko",
            "/lib/modules/virtio_input.ko",
            NULL
        };
        for (int i = 0; mods[i]; i++) {
            if (access(mods[i], F_OK) != 0) continue;
            pid_t pid = fork();
            if (pid == 0) {
                execl("/bin/busybox", "busybox", "insmod", mods[i], NULL);
                _exit(0);
            }
            wait(NULL);
        }
    }
    sleep(1);

    /* ─── Gentoo-style boot finish ─── */
    vga_write("================================================================\n");
    char hostname[64];
    gethostname(hostname, sizeof(hostname));
    char ready_msg[256];
    snprintf(ready_msg, sizeof(ready_msg),
        " * Synth3x-Anon system initialized.  Hostname: %s\n", hostname);
    vga_write(ready_msg);
    vga_write(" * Press ESC in the DE to exit to shell\n\n");

    /* Check /dev/fb0 */
    struct stat st;
    if (stat("/dev/fb0", &st) == 0) {
        vga_write("Synth3x init: /dev/fb0 exists\n");
        /* Print resolution */
        int tmp_fd = open("/dev/fb0", O_RDONLY);
        if (tmp_fd >= 0) {
            struct fb_var_screeninfo vi;
            if (ioctl(tmp_fd, FBIOGET_VSCREENINFO, &vi) == 0) {
                char res[64];
                snprintf(res, sizeof(res), "  fb0: %dx%d %dbpp\n", vi.xres, vi.yres, vi.bits_per_pixel);
                vga_write(res);
            }
            close(tmp_fd);
        }
    } else {
        vga_write("Synth3x init: /dev/fb0 NOT found\n");
    }

    /* List input devices */
    DIR *id = opendir("/dev/input");
    if (id) {
        struct dirent *de;
        while ((de = readdir(id))) {
            if (de->d_name[0] != '.') {
                char buf[64];
                snprintf(buf, sizeof(buf), "  /dev/input/%s\n", de->d_name);
                vga_write(buf);
            }
        }
        closedir(id);
    } else {
        vga_write("  /dev/input: directory not found\n");
    }

    /* Switch to graphics mode */
    int tty = open("/dev/tty0", O_RDWR);
    if (tty >= 0) {
        ioctl(tty, KDSETMODE, KD_GRAPHICS);
        close(tty);
    }

    /* Fork Synth3x DE so we can respawn if it crashes */
    vga_write("Synth3x init: launching Synth3x DE...\n");
    for (;;) {
        pid_t pid = fork();
        if (pid < 0) { sleep(3); continue; }
        if (pid == 0) {
            struct stat de_st;
            if (stat(SYNTH3X_DE, &de_st) == 0) {
                execl(SYNTH3X_DE, "synth3x", NULL);
            }
            /* fallback to shell */
            execl(SHELL, "sh", NULL);
            _exit(1);
        }
        int status;
        waitpid(pid, &status, 0);
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "init: PID %d exit %d, respawn\n", pid, status);
            vga_write(buf);
        }
        sleep(2);
    }
}
