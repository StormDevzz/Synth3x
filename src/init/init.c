/* Synth3x-Anon — init (PID 1) — Console Boot
 * Mounts filesystems, detects hardware, randomizes identity,
 * loads modules, then drops to interactive bash shell.
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
            execl("/bin/busybox", "busybox", "udhcpc", "-i", de->d_name, "-q", NULL);
            _exit(1);
        }
    }
    closedir(d);
}

static void setup_wayland_socket(void) {
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    if (!runtime) {
        mkdir("/tmp", 0755);
        setenv("XDG_RUNTIME_DIR", "/tmp", 1);
    }
}

static void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

#include "../hardware/hw_detect.h"

/* Launch interactive shell with proper TTY setup */
static void launch_shell(void) {
    for (;;) {
        pid_t sh_pid = fork();
        if (sh_pid < 0) { sleep(1); continue; }
        if (sh_pid == 0) {
            setsid();
            int fd = open("/dev/tty1", O_RDWR | O_NOCTTY);
            if (fd >= 0) { ioctl(fd, TIOCSCTTY, 0); }
            if (fd >= 0) { dup2(fd, 0); dup2(fd, 1); dup2(fd, 2); if (fd > 2) close(fd); }
            setenv("HOME", "/root", 1);
            setenv("SHELL", "/bin/bash", 1);
            setenv("USER", "root", 1);
            setenv("LOGNAME", "root", 1);
            setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin", 1);
            setenv("TERM", "linux", 1);
            setenv("HOSTNAME", "synth3x", 1);
            setenv("PS1", "\\[\\033[1;36m\\]synth3x-root\\[\\033[0m\\]:\\[\\033[1;33m\\]\\w\\[\\033[0m\\]\\$ ", 1);
            execl("/bin/bash", "bash", "-l", NULL);
            execl("/bin/sh", "sh", NULL);
            _exit(1);
        }
        int sh_status;
        waitpid(sh_pid, &sh_status, 0);
        if (WIFEXITED(sh_status) && WEXITSTATUS(sh_status) == 0) {
            continue;
        }
        vga_write("\n[shell restarted...]\n");
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin", 1);

    /* Mount essential filesystems */
    mkdir("/proc", 0755); mount("proc", "/proc", "proc", 0, NULL);
    mkdir("/sys", 0755);  mount("sysfs", "/sys", "sysfs", 0, NULL);
    mkdir("/dev", 0755);  mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    mkdir("/tmp", 0755);  mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
    mkdir("/run", 0755);  mount("tmpfs", "/run", "tmpfs", 0, NULL);

    /* Reap zombie children */
    struct sigaction sa = { .sa_handler = sigchld_handler, .sa_flags = SA_RESTART | SA_NOCLDSTOP };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    /* ─── Boot banner ─── */
    vga_write("\n");
    vga_write(" Synth3x Console v" VERSION "\n");
    vga_write("══════════════════════════════════════════════════════════\n");

    /* ─── Mount filesystems ─── */
    vga_write(" * Mounting filesystems ...\n");
    vga_write("   [ OK ]  proc, sysfs, devtmpfs, tmpfs\n");

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
    vga_write(hw_is_vm() ? "   Environment: Virtual Machine\n" : "   Environment: Physical Hardware\n");
    vga_write(hw_has_touchpad() ? "   Touchpad: detected\n" : "   Touchpad: not detected\n");
    vga_write(hw_is_laptop() ? "   Chassis: Laptop\n" : "   Chassis: Desktop\n");
    vga_write(" * Loading hardware modules ...\n");
    hw_auto_configure();
    vga_write("   [ OK ]  hardware modules\n");

    /* ─── Users and groups ─── */
    mkdir("/etc", 0755);
    {
        int fd = open("/etc/passwd", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            const char *pwd = "root:x:0:0:root:/root:/bin/bash\n";
            write(fd, pwd, strlen(pwd)); close(fd);
        }
    }
    {
        int fd = open("/etc/group", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            const char *grp = "root:x:0:\n"; write(fd, grp, strlen(grp)); close(fd);
        }
    }

    /* ─── Network identity ─── */
    vga_write(" * Randomizing network identity ...\n");
    randomize_hostname();
    randomize_all_macs();
    bring_lo_up();
    setup_resolv_conf();
    vga_write("   [ OK ]  MAC + hostname randomized\n");

    /* Wayland socket dir */
    setup_wayland_socket();

    /* ─── Firewall ─── */
    vga_write(" * Starting firewall (nftables) ...\n");
    if (fork() == 0) { execl("/usr/sbin/nft", "nft", "-f", "/etc/nftables.rules", NULL); _exit(1); }
    sleep(1);
    vga_write("   [ OK ]  nftables\n");

    /* ─── DHCP ─── */
    vga_write(" * Starting network interfaces ...\n");
    run_dhcp();
    vga_write("   [ OK ]  DHCP started\n");

    /* ─── Load graphics modules (for compositor later if needed) ─── */
    vga_write(" * Loading graphics modules ...\n");
    const char *mods[] = {
        "/lib/modules/ttm.ko", "/lib/modules/bochs.ko",
        "/lib/modules/virtio_dma_buf.ko", "/lib/modules/virtio-gpu.ko",
        "/lib/modules/psmouse.ko", "/lib/modules/mousedev.ko",
        "/lib/modules/virtio_input.ko",
        NULL
    };
    for (int i = 0; mods[i]; i++) {
        if (access(mods[i], F_OK) != 0) continue;
        pid_t pid = fork();
        if (pid == 0) { execl("/bin/busybox", "busybox", "insmod", mods[i], NULL); _exit(0); }
        else wait(NULL);
    }
    sleep(1);
    vga_write("   [ OK ]  graphics modules\n");

    /* ─── Welcome screen ─── */
    vga_write("\033[2J\033[H");
    vga_write("\n══════════════════════════════════════════════════════════\n");
    vga_write("          SYNTH3X CONSOLE  v" VERSION "\n");
    vga_write("══════════════════════════════════════════════════════════\n\n");
    vga_write(" * Start Installation:\n");
    vga_write("   # synth3x-installer\n\n");
    vga_write(" * WiFi Setup:\n");
    vga_write("   # synth3x-wifi\n");
    vga_write("   # iwctl\n\n");
    vga_write(" * Package Manager:\n");
    vga_write("   # emerge --ask <package>\n\n");
    vga_write(" * Download Files:\n");
    vga_write("   # synth3x-downloader\n\n");
    vga_write(" * AmnesiaDE Desktop:\n");
    vga_write("   # synth3x\n\n");
    vga_write(" * Help:\n");
    vga_write("   # synth3x-help\n\n");
    vga_write("══════════════════════════════════════════════════════════\n\n");

    /* ─── Shell config files ─── */
    mkdir("/root", 0700);

    int brc = open("/etc/bashrc", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (brc >= 0) {
        const char *rc =
            "PS1='\\[\\033[1;36m\\]synth3x-root\\[\\033[0m\\]:\\[\\033[1;33m\\]\\w\\[\\033[0m\\]\\$ '\n"
            "export PS1\n"
            "alias ls='ls --color=auto'\n"
            "alias ll='ls -la'\n"
            "alias grep='grep --color=auto'\n"
            "alias install='synth3x-installer'\n"
            "alias wifi='synth3x-wifi'\n"
            "alias dl='synth3x-downloader'\n"
            "alias help='synth3x-help'\n"
            "alias update='emerge --sync && emerge -uDN @world'\n"
            "export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin\n"
            "export TERM=linux\n"
            "export HOME=/root\n"
            "export HISTSIZE=10000\n"
            "export HISTFILESIZE=20000\n"
            "shopt -s histappend\n"
            "shopt -s checkwinsize\n";
        write(brc, rc, strlen(rc));
        close(brc);
    }

    int prof = open("/etc/profile", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (prof >= 0) {
        const char *p =
            "# Synth3x Console\n"
            "export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin\n"
            "export TERM=linux\n"
            "export HOME=/root\n"
            "[ -f /etc/bashrc ] && . /etc/bashrc\n";
        write(prof, p, strlen(p));
        close(prof);
    }

    int rbp = open("/root/.bash_profile", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (rbp >= 0) {
        const char *bp =
            "[ -f /etc/bashrc ] && . /etc/bashrc\n"
            "[ -f /root/.bashrc ] && . /root/.bashrc\n";
        write(rbp, bp, strlen(bp));
        close(rbp);
    }

    int rbrc = open("/root/.bashrc", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (rbrc >= 0) {
        const char *rc2 =
            "PS1='\\[\\033[1;36m\\]synth3x-root\\[\\033[0m\\]:\\[\\033[1;33m\\]\\w\\[\\033[0m\\]\\$ '\n"
            "export PS1\n"
            "alias ls='ls --color=auto'\n"
            "alias ll='ls -la'\n"
            "alias grep='grep --color=auto'\n"
            "alias install='synth3x-installer'\n"
            "alias wifi='synth3x-wifi'\n"
            "alias dl='synth3x-downloader'\n"
            "alias help='synth3x-help'\n"
            "export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin\n"
            "export TERM=linux\n";
        write(rbrc, rc2, strlen(rc2));
        close(rbrc);
    }

    /* ─── Drop to shell (blocks forever) ─── */
    launch_shell();

    return 0;
}
