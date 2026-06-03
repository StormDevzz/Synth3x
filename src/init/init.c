/* Synth3x OS v0.9 — init (PID 1) — Wayland Compositor Boot
 * Mounts filesystems, detects hardware, randomizes identity,
 * loads modules, then launches the Wayland compositor.
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
#include <linux/kd.h>

#define COMPOSITOR  "/usr/bin/synth3x"
#define SHELL       "/bin/sh"

#include "../hardware/hw_detect.h"

static void vga_write(const char *s) {
    int fd = open("/dev/tty0", O_WRONLY);
    if (fd >= 0) { write(fd, s, strlen(s)); close(fd); }
    int sf = open("/dev/ttyS0", O_WRONLY);
    if (sf >= 0) { write(sf, s, strlen(s)); close(sf); }
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
    for (int i = 1; i < 6; i++)
        ifr.ifr_hwaddr.sa_data[i] = rand() % 256;
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
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 ||
            strcmp(de->d_name, "lo") == 0) continue;
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

static int check_cmdline_installer(void) {
    int fd = open("/proc/cmdline", O_RDONLY);
    if (fd < 0) return 0;
    char buf[1024];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    char *p = buf;
    while ((p = strstr(p, "installer"))) {
        int before = (p == buf || *(p - 1) == ' ' || *(p - 1) == '\n' || *(p - 1) == '\t');
        int after = (*(p + 9) == '\0' || *(p + 9) == ' ' || *(p + 9) == '\n' || *(p + 9) == '\t');
        if (before && after) return 1;
        p += 9;
    }
    return 0;
}

static void run_dhcp(void) {
    DIR *d = opendir("/sys/class/net");
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d))) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 ||
            strcmp(de->d_name, "lo") == 0 || strstr(de->d_name, "docker") ||
            strstr(de->d_name, "veth") || strstr(de->d_name, "br-")) continue;
        pid_t pid = fork();
        if (pid == 0) {
            execl("/sbin/dhcpcd", "dhcpcd", "-q", de->d_name, NULL);
            execl("/bin/busybox", "busybox", "udhcpc", "-i", de->d_name, "-b", "-q", "-f", NULL);
            _exit(1);
        }
    }
    closedir(d);
}

/* ─── Wayland socket setup in XDG_RUNTIME_DIR ─── */
static void setup_wayland_socket(void) {
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    if (!runtime) {
        /* Create a runtime dir for the Wayland socket */
        runtime = "/tmp";
        mkdir("/tmp", 0755);
    }
    setenv("XDG_RUNTIME_DIR", runtime, 1);
    setenv("WAYLAND_DISPLAY", "wayland-0", 1);
    unlink("/tmp/wayland-0");
    vga_write("init: WAYLAND_DISPLAY=wayland-0\n");
}

int main(int argc, char *argv[]) {
    setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin", 1);

    vga_write("\n");
    vga_write(" Synth3x-Anon OS v0.9  —  Wayland Compositor\n");
    vga_write("================================================================\n");
    vga_write(" * Mounting filesystems ...\n");

    mkdir("/proc", 0755); mount("proc", "/proc", "proc", 0, NULL);
    vga_write("   [ OK ]  proc\n");
    mkdir("/sys", 0755);  mount("sysfs", "/sys", "sysfs", 0, NULL);
    vga_write("   [ OK ]  sysfs\n");
    mkdir("/dev", 0755);  mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    vga_write("   [ OK ]  devtmpfs\n");
    mkdir("/tmp", 0755);  mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
    vga_write("   [ OK ]  tmpfs\n");
    mkdir("/run", 0755);  mount("tmpfs", "/run", "tmpfs", 0, NULL);
    vga_write("   [ OK ]  tmpfs (/run)\n");

    /* ─── Hardware detection ─── */
    vga_write(" * Detecting hardware ...\n");
    char cpu_vendor[16] = "";
    hw_cpuid_vendor(cpu_vendor);
    char cpu_brand[64] = "";
    hw_cpuid_brand_string(cpu_brand);
    char hw_buf[128];
    snprintf(hw_buf, sizeof(hw_buf), "   CPU: %s  (%s)\n", cpu_vendor, cpu_brand);
    vga_write(hw_buf);

    char dmi_vendor[64] = "", dmi_product[64] = "";
    if (hw_dmi_vendor(dmi_vendor, sizeof(dmi_vendor)) == 0) {
        snprintf(hw_buf, sizeof(hw_buf), "   Vendor: %s\n", dmi_vendor);
        vga_write(hw_buf);
    }
    if (hw_dmi_product(dmi_product, sizeof(dmi_product)) == 0) {
        snprintf(hw_buf, sizeof(hw_buf), "   Model: %s\n", dmi_product);
        vga_write(hw_buf);
    }

    vga_write(hw_has_touchpad() ? "   Touchpad: detected\n" : "   Touchpad: not detected\n");
    vga_write(hw_is_laptop() ? "   Chassis: Laptop\n" : "   Chassis: Desktop\n");

    vga_write(" * Loading hardware modules ...\n");
    hw_auto_configure();
    vga_write("   [ OK ]  hardware modules\n");

    /* Setup users and groups */
    mkdir("/etc", 0755);
    int pwd_fd = open("/etc/passwd", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (pwd_fd >= 0) {
        const char *pwd = "root:x:0:0:root:/root:/bin/sh\ntor:x:100:100:tor:/var/lib/tor:/bin/sh\n";
        write(pwd_fd, pwd, strlen(pwd)); close(pwd_fd);
    }
    int grp_fd = open("/etc/group", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (grp_fd >= 0) {
        const char *grp = "root:x:0:\ntor:x:100:\n"; write(grp_fd, grp, strlen(grp)); close(grp_fd);
    }
    mkdir("/var", 0755); mkdir("/var/lib", 0755);
    mkdir("/var/lib/tor", 0700); mkdir("/var/log", 0755);
    mkdir("/run", 0755); chown("/var/lib/tor", 100, 100);

    /* Network identity randomization */
    vga_write(" * Randomizing network identity ...\n");
    randomize_hostname();
    randomize_all_macs();
    bring_lo_up();
    setup_resolv_conf();
    vga_write("   [ OK ]  MAC + hostname randomized\n");

    /* Wayland socket */
    setup_wayland_socket();

    /* Firewall */
    vga_write(" * Starting firewall (nftables) ...\n");
    if (fork() == 0) {
        execl("/usr/sbin/nft", "nft", "-f", "/etc/nftables.rules", NULL);
        _exit(1);
    }
    sleep(1);
    vga_write("   [ OK ]  nftables\n");

    /* DHCP */
    vga_write(" * Starting network interfaces ...\n");
    run_dhcp();
    vga_write("   [ OK ]  DHCP started\n");

    /* Tor is lazy-started */
    vga_write(" * Tor: lazy (run 'tor-start' to enable)\n");

    if (check_cmdline_installer()) {
        vga_write("================================================================\n");
        vga_write(" * Entering Terminal Installer mode...\n");
        vga_write("================================================================\n");
        for (;;) {
            pid_t pid = fork();
            if (pid == 0) {
                execl("/usr/bin/synth3x-installer", "synth3x-installer", NULL);
                _exit(1);
            }
            int status;
            waitpid(pid, &status, 0);
            vga_write("\n[!] Installer finished or exited. Starting fallback shell...\n");
            pid_t sh_pid = fork();
            if (sh_pid == 0) {
                execl("/bin/sh", "sh", NULL);
                _exit(1);
            }
            waitpid(sh_pid, &status, 0);
            sleep(2);
        }
    }

    /* Load graphics modules */
    vga_write(" * Loading graphics modules ...\n");
    const char *mods[] = {
        "/lib/modules/ttm.ko", "/lib/modules/bochs.ko",
        "/lib/modules/virtio-gpu.ko", "/lib/modules/psmouse.ko",
        "/lib/modules/mousedev.ko", "/lib/modules/virtio_input.ko",
        NULL
    };
    for (int i = 0; mods[i]; i++) {
        if (access(mods[i], F_OK) != 0) continue;
        pid_t pid = fork();
        if (pid == 0) { execl("/bin/busybox", "busybox", "insmod", mods[i], NULL); _exit(0); }
        else wait(NULL);
    }
    sleep(1);

    /* Check DRM device */
    vga_write(" * Checking display devices ...\n");
    if (access("/dev/dri/card0", F_OK) == 0) {
        vga_write("   [ OK ]  /dev/dri/card0 (DRM)\n");
    } else {
        vga_write("   [!] /dev/dri/card0 not found, trying fbdev\n");
        if (access("/dev/fb0", F_OK) == 0) {
            vga_write("   [ OK ]  /dev/fb0 (fbdev fallback)\n");
        }
    }

    vga_write("================================================================\n");
    char hostname[64];
    gethostname(hostname, sizeof(hostname));
    snprintf(hw_buf, sizeof(hw_buf),
             " * Synth3x-Anon v0.9 initialized.  Hostname: %s\n", hostname);
    vga_write(hw_buf);
    vga_write(" * Wayland compositor starting...\n\n");

    /* Switch to graphics mode */
    int tty = open("/dev/tty0", O_RDWR);
    if (tty >= 0) {
        ioctl(tty, KDSETMODE, KD_GRAPHICS);
        close(tty);
    }

    /* Launch the Wayland compositor */
    vga_write("Synth3x init: launching Wayland compositor...\n");
    for (;;) {
        pid_t pid = fork();
        if (pid < 0) { sleep(3); continue; }
        if (pid == 0) {
            struct stat st;
            if (stat(COMPOSITOR, &st) == 0) {
                execl(COMPOSITOR, "synth3x", NULL);
            }
            execl(SHELL, "sh", NULL);
            _exit(1);
        }
        int status;
        waitpid(pid, &status, 0);
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "init: compositor PID %d exit %d, respawn\n",
                     pid, status);
            vga_write(buf);
        }
        sleep(2);
    }
}
