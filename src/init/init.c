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
#define SHELL       "/bin/bash"

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

#include <sys/socket.h>
#include <net/if.h>
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
        const char *conf = "nameserver 127.0.0.1\noptions use-vc\n";
        write(fd, conf, strlen(conf));
        close(fd);
    }
}

static void run_dhcp(void) {
    DIR *d = opendir("/sys/class/net");
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d))) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, "lo") == 0)
            continue;
        pid_t pid = fork();
        if (pid == 0) {
            execl("/bin/busybox", "busybox", "udhcpc", "-i", de->d_name, "-b", "-q", NULL);
            _exit(1);
        }
    }
    closedir(d);
}

/* ─── Main ─── */
int main(int argc, char *argv[]) {
    setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);
    vga_write("Synth3x init: starting\n");

    /* Mount filesystems */
    mkdir("/proc", 0755); mount("proc", "/proc", "proc", 0, NULL);
    mkdir("/sys", 0755);  mount("sysfs", "/sys", "sysfs", 0, NULL);
    mkdir("/dev", 0755);  mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    mkdir("/tmp", 0755);  mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
    
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
    chown("/var/lib/tor", 100, 100);
    chown("/var/log/tor", 100, 100);

    vga_write("Synth3x init: randomizing identity...\n");
    randomize_hostname();
    randomize_all_macs();
    bring_lo_up();
    setup_resolv_conf();

    vga_write("Synth3x init: starting firewall...\n");
    if (fork() == 0) {
        execl("/usr/sbin/nft", "nft", "-f", "/etc/nftables.rules", NULL);
        _exit(1);
    }
    sleep(1);

    vga_write("Synth3x init: starting network interfaces...\n");
    run_dhcp();

    vga_write("Synth3x init: starting Tor...\n");
    if (fork() == 0) {
        execl("/usr/bin/tor", "tor", "-f", "/etc/tor/torrc", "--runasdaemon", "0", NULL);
        _exit(1);
    }

    vga_write("Synth3x init: /dev mounted\n");

    /* Check /dev/fb0 */
    struct stat st;
    if (stat("/dev/fb0", &st) == 0) {
        vga_write("Synth3x init: /dev/fb0 exists\n");
    } else {
        vga_write("Synth3x init: /dev/fb0 NOT found\n");
    }

    /* Switch to graphics mode */
    int tty = open("/dev/tty0", O_RDWR);
    if (tty >= 0) {
        ioctl(tty, KDSETMODE, KD_GRAPHICS);
        close(tty);
        vga_write("Synth3x init: graphics mode set\n");
    }

    /* Try to launch Synth3x DE directly */
    {
        struct stat de_st;
        if (stat(SYNTH3X_DE, &de_st) == 0) {
            vga_write("Synth3x init: launching Synth3x DE...\n");
            execl(SYNTH3X_DE, "synth3x", NULL);
        }
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
