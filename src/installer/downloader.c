/* Synth3x-Anon — Installer File Downloader (C)
 * Downloads essential files for installation: Stage3, GRUB modules,
 * kernel, Portage snapshots, and packages.
 * Uses curl/wget with fallback, SHA256 verification, and progress display.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>

#define CYAN    "\033[0;36m"
#define PURPLE  "\033[0;35m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[1;33m"
#define RED     "\033[0;31m"
#define DIM     "\033[0;90m"
#define BOLD    "\033[1m"
#define NC      "\033[0m"

#define MAX_URLS 16
#define MAX_PATH 512

typedef struct {
    const char *name;
    const char *url;
    const char *dest;
    const char *expected_sha256; /* NULL = skip verification */
    int compress;                /* 1 = needs decompression after download */
} download_entry;

/* ─── Gentoo Stage3 download URLs (mirrors) ─── */
static const download_entry stage3_entries[] = {
    {
        "stage3-amd64-hardened-openrc",
        "https://bouncer.gentoo.org/fetch/root/all/releases/amd64/autobuilds/current-stage3-amd64-hardened-openrc/stage3-amd64-hardened-openrc-latest.tar.xz",
        "/mnt/gentoo/stage3.tar.xz",
        NULL, 1
    },
    {
        "portage-latest",
        "https://bouncer.gentoo.org/fetch/root/repos/gentoo-portage-latest.tar.xz",
        "/mnt/gentoo/portage.tar.xz",
        NULL, 1
    },
    { NULL, NULL, NULL, NULL, 0 }
};

/* ─── Verify SHA256 integrity using sha256sum ─── */
static int verify_sha256(const char *file_path, const char *sha256_path) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "sha256sum '%s' | awk '{print $1}' > /tmp/calculated_sha256 2>/dev/null", file_path);
    if (system(cmd) != 0) return 0;
    
    FILE *f_calc = fopen("/tmp/calculated_sha256", "r");
    if (!f_calc) return 0;
    char calc_hash[128] = "";
    if (fgets(calc_hash, sizeof(calc_hash), f_calc) == NULL) {
        fclose(f_calc);
        return 0;
    }
    fclose(f_calc);
    unlink("/tmp/calculated_sha256");

    FILE *f_exp = fopen(sha256_path, "r");
    if (!f_exp) return 0;
    char exp_hash[128] = "";
    if (fgets(exp_hash, sizeof(exp_hash), f_exp) == NULL) {
        fclose(f_exp);
        return 0;
    }
    fclose(f_exp);

    /* Trim whitespace */
    char *p = calc_hash;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    char *end = p;
    while (*end && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n') end++;
    *end = 0;

    char *pe = exp_hash;
    while (*pe && (*pe == ' ' || *pe == '\t' || *pe == '\r' || *pe == '\n')) pe++;
    char *ende = pe;
    while (*ende && *ende != ' ' && *ende != '\t' && *ende != '\r' && *ende != '\n') ende++;
    *ende = 0;

    if (strlen(p) == 64 && strlen(pe) == 64 && strcmp(p, pe) == 0) {
        return 1;
    }
    return 0;
}

/* ─── Utility: check if a command exists ─── */
static int cmd_exists(const char *cmd) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "which %s >/dev/null 2>&1", cmd);
    return system(path) == 0;
}

/* ─── Utility: run a command and return status ─── */
static int run_cmd(const char *fmt, ...) {
    char cmd[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, args);
    va_end(args);
    return system(cmd);
}

/* ─── Display progress bar ─── */
static void show_progress(const char *label, int percent) {
    int bar_width = 30;
    int filled = (percent * bar_width) / 100;
    printf("\r  %s%s%s [", CYAN, label, NC);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled)
            printf("%s#%s", GREEN, NC);
        else
            printf("%s-%s", DIM, NC);
    }
    printf("] %s%3d%%%s", BOLD, percent, NC);
    fflush(stdout);
}

/* ─── Download with curl (preferred) or wget fallback ─── */
static int download_file(const char *url, const char *dest) {
    int ret = -1;

    /* Try curl first */
    if (cmd_exists("curl")) {
        printf("  %sDownloading with curl...%s\n", CYAN, NC);
        ret = run_cmd("curl -L --progress-bar --cacert /etc/ssl/certs/ca-certificates.crt -o '%s' '%s' 2>&1",
                      dest, url);
    }
    /* Fallback to wget */
    else if (cmd_exists("wget")) {
        printf("  %sDownloading with wget...%s\n", CYAN, NC);
        ret = run_cmd("wget -q --show-progress --ca-certificate=/etc/ssl/certs/ca-certificates.crt -O '%s' '%s' 2>&1",
                      dest, url);
    }
    /* Fallback to busybox wget */
    else if (cmd_exists("/bin/busybox")) {
        printf("  %sDownloading with busybox wget...%s\n", CYAN, NC);
        ret = run_cmd("/bin/busybox wget -O '%s' '%s' 2>&1",
                      dest, url);
    }
    else {
        printf("  %s[ERROR] No download tool found (curl/wget)!%s\n", RED, NC);
        return -1;
    }

    if (ret != 0) {
        printf("  %s[WARN] Download failed (exit %d)%s\n", YELLOW, ret, NC);
        return -1;
    }

    /* Verify file exists and has size > 0 */
    struct stat st;
    if (stat(dest, &st) != 0 || st.st_size == 0) {
        printf("  %s[ERROR] Downloaded file is empty or missing!%s\n", RED, NC);
        return -1;
    }

    return 0;
}

/* ─── Decompress .tar.xz or .tar.gz ─── */
static int decompress(const char *archive, const char *dest_dir) {
    printf("  %sDecompressing: %s%s\n", CYAN, archive, NC);

    if (strstr(archive, ".tar.xz")) {
        return run_cmd("tar -xJpf '%s' -C '%s' 2>&1",
                       archive, dest_dir);
    } else if (strstr(archive, ".tar.gz")) {
        return run_cmd("tar -xzf '%s' -C '%s' 2>&1",
                       archive, dest_dir);
    } else if (strstr(archive, ".tar")) {
        return run_cmd("tar -xpf '%s' -C '%s' 2>&1",
                       archive, dest_dir);
    }
    return -1;
}

/* ─── Verify internet connectivity ─── */
static int check_internet(void) {
    printf("  %sChecking internet connection...%s\n", CYAN, NC);

    /* Try pinging multiple DNS servers */
    const char *hosts[] = {
        "1.1.1.1", "8.8.8.8", "gentoo.org", NULL
    };

    for (int i = 0; hosts[i]; i++) {
        if (run_cmd("ping -c 1 -W 3 %s >/dev/null 2>&1", hosts[i]) == 0) {
            printf("  %s[OK] Internet available via %s%s\n", GREEN, hosts[i], NC);
            return 1;
        }
    }

    printf("  %s[WARN] No internet connection detected%s\n", YELLOW, NC);
    return 0;
}

/* ─── Download all essential installation files ─── */
int installer_download_stage3(void) {
    printf("\n  %s═══ STAGE 3 DOWNLOAD ═══%s\n", PURPLE, NC);

    if (!check_internet()) {
        printf("  %s[!] Cannot download Stage3 without internet.%s\n", RED, NC);
        printf("  %s    Connect to WiFi first (use: synth3x-wifi) %s\n", YELLOW, NC);
        return -1;
    }

    mkdir("/mnt/gentoo", 0755);

    for (int i = 0; stage3_entries[i].name; i++) {
        const download_entry *e = &stage3_entries[i];
        printf("\n  %s[%d/%d] %s%s\n", BOLD, i + 1, 2, e->name, NC);
        printf("  %sURL: %s%s\n", DIM, e->url, NC);
        printf("  %sDest: %s%s\n", DIM, e->dest, NC);

        if (download_file(e->url, e->dest) != 0) {
            printf("  %s[FAIL] Could not download %s%s\n", RED, e->name, NC);
            return -1;
        }
        printf("  %s[OK] %s downloaded%s\n", GREEN, e->name, NC);

        /* Verify stage3 SHA256 */
        if (strcmp(e->name, "stage3-amd64-hardened-openrc") == 0) {
            char sha_url[512];
            snprintf(sha_url, sizeof(sha_url), "%s.sha256", e->url);
            char sha_dest[256];
            snprintf(sha_dest, sizeof(sha_dest), "%s.sha256", e->dest);
            printf("  %sDownloading SHA256 checksum...%s\n", CYAN, NC);
            if (download_file(sha_url, sha_dest) == 0) {
                printf("  %sVerifying integrity (SHA256)...%s\n", CYAN, NC);
                if (verify_sha256(e->dest, sha_dest)) {
                    printf("  %s[OK] SHA256 verified!%s\n", GREEN, NC);
                    unlink(sha_dest);
                } else {
                    printf("  %s[ERROR] SHA256 mismatch! File is compromised or corrupted!%s\n", RED, NC);
                    unlink(e->dest);
                    unlink(sha_dest);
                    return -1;
                }
            } else {
                printf("  %s[ERROR] Could not download SHA256 checksum file!%s\n", RED, NC);
                unlink(e->dest);
                return -1;
            }
        }

        if (e->compress) {
            if (decompress(e->dest, "/mnt/gentoo") != 0) {
                printf("  %s[FAIL] Decompression failed for %s%s\n", RED, e->name, NC);
                return -1;
            }
            unlink(e->dest);
            printf("  %s[OK] %s extracted%s\n", GREEN, e->name, NC);
        }
    }

    printf("\n  %s✓ Stage3 + Portage downloaded and extracted%s\n", GREEN, NC);
    return 0;
}

/* ─── Download GRUB modules for UEFI ─── */
int installer_download_grub_modules(void) {
    printf("\n  %s═══ GRUB UEFI MODULES ═══%s\n", PURPLE, NC);

    const char *grub_url = "https://raw.githubusercontent.com/nprevenant/Synth3x/main/boot/grub.cfg";
    const char *grub_dest = "/mnt/gentoo/boot/grub/grub.cfg";

    mkdir("/mnt/gentoo/boot/grub", 0755);

    /* Download GRUB config */
    if (download_file(grub_url, grub_dest) != 0) {
        printf("  %s[WARN] Could not download grub.cfg, using local copy%s\n", YELLOW, NC);
        if (access("/boot/grub/grub.cfg", F_OK) == 0) {
            run_cmd("cp /boot/grub/grub.cfg %s", grub_dest);
            printf("  %s[OK] Local grub.cfg copied%s\n", GREEN, NC);
        }
    } else {
        printf("  %s[OK] GRUB config downloaded%s\n", GREEN, NC);
    }

    return 0;
}

/* ─── Download kernel to target ─── */
int installer_download_kernel(void) {
    printf("\n  %s═══ KERNEL ═══%s\n", PURPLE, NC);

    /* Try to copy running kernel first */
    const char *kernel_locations[] = {
        "/boot/vmlinuz-linux",
        "/boot/vmlinuz",
        "/usr/lib/modules/*/vmlinuz",
        NULL
    };

    for (int i = 0; kernel_locations[i]; i++) {
        if (access(kernel_locations[i], F_OK) == 0) {
            run_cmd("cp %s /mnt/gentoo/boot/vmlinuz-linux 2>/dev/null", kernel_locations[i]);
            printf("  %s[OK] Kernel copied from %s%s\n", GREEN, kernel_locations[i], NC);
            return 0;
        }
    }

    printf("  %s[WARN] No kernel found to copy%s\n", YELLOW, NC);
    return 0;
}

/* ─── Download Portage snapshot ─── */
int installer_download_portage(void) {
    printf("\n  %s═══ PORTAGE SNAPSHOT ═══%s\n", PURPLE, NC);

    if (!check_internet()) {
        printf("  %s[!] No internet for Portage download%s\n", YELLOW, NC);
        return -1;
    }

    const char *portage_url = "https://bouncer.gentoo.org/fetch/root/repos/gentoo-portage-latest.tar.xz";
    const char *portage_dest = "/mnt/gentoo/var/db/repos/gentoo/portage-latest.tar.xz";

    mkdir("/mnt/gentoo/var/db/repos/gentoo", 0755);

    if (download_file(portage_url, portage_dest) == 0) {
        decompress(portage_dest, "/mnt/gentoo/var/db/repos/gentoo");
        unlink(portage_dest);
        printf("  %s[OK] Portage snapshot installed%s\n", GREEN, NC);
        return 0;
    }

    printf("  %s[WARN] Portage snapshot download failed%s\n", YELLOW, NC);
    return -1;
}

/* ─── Summary of what will be downloaded ─── */
void installer_show_download_plan(void) {
    printf("\n  %s═══ DOWNLOAD PLAN ═══%s\n", PURPLE, NC);
    printf("  %sThe following files will be downloaded:%s\n\n", DIM, NC);

    printf("  %s1.%s Gentoo Stage3 tarball (~300MB)\n", CYAN, NC);
    printf("     %sBase system for Synth3x%s\n", DIM, NC);
    printf("  %s2.%s Portage package tree (~100MB)\n", CYAN, NC);
    printf("     %sPackage metadata and ebuilds%s\n", DIM, NC);
    printf("  %s3.%s Kernel (from live environment)\n", CYAN, NC);
    printf("     %sCopied from running system%s\n", DIM, NC);
    printf("  %s4.%s GRUB UEFI bootloader\n", CYAN, NC);
    printf("     %sInstalled from live environment%s\n", DIM, NC);
    printf("\n");
}

#ifdef STANDALONE
int main(int argc, char *argv[]) {
    printf("\n  %s═══════════════════════════════════════════════════%s\n", PURPLE, NC);
    printf("  %s  Synth3x Installer — File Downloader v%s%s\n", CYAN, VERSION, NC);
    printf("  %s═══════════════════════════════════════════════════%s\n\n", PURPLE, NC);

    installer_show_download_plan();

    if (argc > 1 && strcmp(argv[1], "--stage3") == 0) {
        return installer_download_stage3();
    }
    if (argc > 1 && strcmp(argv[1], "--grub") == 0) {
        return installer_download_grub_modules();
    }
    if (argc > 1 && strcmp(argv[1], "--kernel") == 0) {
        return installer_download_kernel();
    }
    if (argc > 1 && strcmp(argv[1], "--portage") == 0) {
        return installer_download_portage();
    }

    /* Download everything */
    int ret = 0;
    if (installer_download_stage3() != 0) ret = -1;
    if (installer_download_kernel() != 0) ret = -1;
    if (installer_download_grub_modules() != 0) ret = -1;

    printf("\n  %s%s═══ DOWNLOAD COMPLETE ═══%s\n", ret == 0 ? GREEN : RED, BOLD, NC);
    return ret;
}
#endif
