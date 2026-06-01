/* Synth3x OS — syn — Gentoo-style package manager
 *
 * Usage:
 *   sudo syn inst <package>   — Install package (binary)
 *   sudo syn binary <package> — Install binary package directly
 *   sudo syn remove <package> — Remove package
 *   syn list                  — List installed packages
 *   syn search <query>        — Search packages
 *   syn update                — Update package database
 *   syn info <package>        — Show package info
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#define PKG_DB_DIR  "/var/db/syn"
#define PKG_CACHE   "/var/cache/syn"
#define PKG_REPO    "https://packages.synth3x.org"

/* ─── ANSI colors ─── */
#define CYAN    "\033[0;36m"
#define PURPLE  "\033[0;35m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[1;33m"
#define RED     "\033[0;31m"
#define NC      "\033[0m"

/* ─── Banner ─── */
static void show_banner(void) {
    printf("\n");
    printf("  " PURPLE ".ggg" CYAN ".g\"\"\"bgd    " GREEN "gdBBg" CYAN "   " RED "          \n");
    printf(" " PURPLE "dP\"\" " CYAN "dP   `\"db  " GREEN "dP'  `Yo" CYAN "            \n");
    printf(" " PURPLE "TM" CYAN "   dP    dP' " GREEN "dP    `Yb" CYAN "   " YELLOW "syn — Package Manager" CYAN "\n");
    printf(" " PURPLE "MM" CYAN "   Yb. ,dP'  " GREEN "Yb    dP" CYAN "   " PURPLE "Synth3x Gentoo OS" CYAN "\n");
    printf(" " PURPLE "MM" CYAN "    Y\"\"bdP     " GREEN "\"YbgdP\"" CYAN "    " RED "Binary & Source" CYAN "\n");
    printf(" " PURPLE "MM" CYAN "                                               " CYAN "\n");
    printf(" " PURPLE "MM" CYAN "   Usage: syn [inst|binary|remove|list|search|update|info]\n" NC);
    printf("\n");
}

/* ─── Database helpers ─── */
static void ensure_db(void) {
    mkdir(PKG_DB_DIR, 0755);
    mkdir(PKG_CACHE, 0755);
}

static int is_installed(const char *pkg) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", PKG_DB_DIR, pkg);
    return access(path, F_OK) == 0;
}

static void mark_installed(const char *pkg, const char *version) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", PKG_DB_DIR, pkg);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        dprintf(fd, "package: %s\nversion: %s\ninstalled: %ld\n", pkg, version, time(NULL));
        close(fd);
    }
}

static void mark_removed(const char *pkg) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", PKG_DB_DIR, pkg);
    unlink(path);
}

/* ─── Package database (built-in) ─── */
typedef struct {
    const char *name;
    const char *version;
    const char *desc;
    const char *deps;
    const char *url;
    const char *binary;  /* URL to binary tarball */
} PkgEntry;

static PkgEntry pkg_db[] = {
    {"telegram-desktop", "4.16.8", "Telegram Desktop messenger client", "libc,qt6", "https://desktop.telegram.org", "https://packages.synth3x.org/binary/telegram-desktop-4.16.8.tar.xz"},
    {"firefox", "128.0", "Firefox web browser", "libc,gtk3,dbus", "https://mozilla.org", "https://packages.synth3x.org/binary/firefox-128.0.tar.xz"},
    {"vscodium", "1.92.0", "VS Code editor (open source)", "libc,glib2,gtk3", "https://vscodium.com", "https://packages.synth3x.org/binary/vscodium-1.92.0.tar.xz"},
    {"vim", "9.1", "Advanced text editor", "libc,ncurses", "https://vim.org", "https://packages.synth3x.org/binary/vim-9.1.tar.xz"},
    {"htop", "3.3.0", "Interactive process viewer", "libc,ncurses", "https://htop.dev", "https://packages.synth3x.org/binary/htop-3.3.0.tar.xz"},
    {"neofetch", "7.1.0", "System info display tool", "libc,bash", "https://github.com/dylanaraps/neofetch", "https://packages.synth3x.org/binary/neofetch-7.1.0.tar.xz"},
    {"git", "2.45.0", "Distributed version control", "libc,zlib,curl", "https://git-scm.com", "https://packages.synth3x.org/binary/git-2.45.0.tar.xz"},
    {"wget", "1.24.5", "Network file downloader", "libc,openssl", "https://gnu.org/software/wget", "https://packages.synth3x.org/binary/wget-1.24.5.tar.xz"},
    {"nano", "8.0", "Simple text editor", "libc,ncurses", "https://nano-editor.org", "https://packages.synth3x.org/binary/nano-8.0.tar.xz"},
    {"gcc", "14.1.0", "GNU C/C++ compiler", "libc,binutils", "https://gcc.gnu.org", "https://packages.synth3x.org/binary/gcc-14.1.0.tar.xz"},
    {"python", "3.12.4", "Python programming language", "libc,ssl,zlib", "https://python.org", "https://packages.synth3x.org/binary/python-3.12.4.tar.xz"},
    {"nodejs", "22.3.0", "JavaScript runtime", "libc,ssl", "https://nodejs.org", "https://packages.synth3x.org/binary/nodejs-22.3.0.tar.xz"},
    {"rustc", "1.79.0", "Rust compiler", "libc,binutils", "https://rust-lang.org", "https://packages.synth3x.org/binary/rustc-1.79.0.tar.xz"},
    {"go", "1.22.4", "Go programming language", "libc", "https://go.dev", "https://packages.synth3x.org/binary/go-1.22.4.tar.xz"},
    {"bind", "9.18.27", "DNS server tools", "libc,ssl", "https://isc.org/bind", "https://packages.synth3x.org/binary/bind-9.18.27.tar.xz"},
    {"nginx", "1.26.1", "High-performance web server", "libc,ssl,zlib", "https://nginx.org", "https://packages.synth3x.org/binary/nginx-1.26.1.tar.xz"},
    {"docker", "26.1.4", "Container runtime", "libc,iptables", "https://docker.com", "https://packages.synth3x.org/binary/docker-26.1.4.tar.xz"},
    {"ibus", "1.5.30", "Input method framework", "libc,gtk3,dbus", "https://github.com/ibus/ibus", "https://packages.synth3x.org/binary/ibus-1.5.30.tar.xz"},
    {NULL, NULL, NULL, NULL, NULL, NULL}
};

static PkgEntry *find_pkg(const char *name) {
    for (int i = 0; pkg_db[i].name; i++) {
        if (strcmp(pkg_db[i].name, name) == 0)
            return &pkg_db[i];
    }
    return NULL;
}

/* ─── Installation simulation ─── */
static int syn_install(const char *pkg_name, int binary_mode) {
    ensure_db();

    if (is_installed(pkg_name)) {
        printf("  " YELLOW "[!] %s is already installed." NC "\n", pkg_name);
        return 0;
    }

    PkgEntry *pkg = find_pkg(pkg_name);
    if (!pkg) {
        printf("  " RED "[!] Package '%s' not found in repository." NC "\n", pkg_name);
        printf("  " YELLOW "    Available packages:" NC "\n");
        for (int i = 0; pkg_db[i].name; i++)
            printf("    - %s (%s)\n", pkg_db[i].name, pkg_db[i].desc);
        return 1;
    }

    printf("\n");
    printf("  " CYAN "╔════════════════════════════════════════════════════╗" NC "\n");
    printf("  " CYAN "║" NC "  Package:  " GREEN "%s" NC "                      " CYAN "║" NC "\n", pkg->name);
    printf("  " CYAN "║" NC "  Version:  %s                          " CYAN "║" NC "\n", pkg->version);
    printf("  " CYAN "║" NC "  Desc:     %s" NC "     " CYAN "║" NC "\n", pkg->desc);
    printf("  " CYAN "║" NC "  Deps:     %s                        " CYAN "║" NC "\n", pkg->deps);
    printf("  " CYAN "║" NC "  Mode:     %s                         " CYAN "║" NC "\n", binary_mode ? "Binary" : "Source");
    printf("  " CYAN "╚════════════════════════════════════════════════════╝" NC "\n");
    printf("\n");

    /* Simulated download + install with progress */
    printf("  " CYAN ">>>" NC " Resolving dependencies...\n");
    if (pkg->deps && strcmp(pkg->deps, "libc") != 0) {
        printf("  " CYAN ">>>" NC " Installing dependencies: %s\n", pkg->deps);
    }
    printf("  " CYAN ">>>" NC " Fetching " NC "%s" NC "\n", binary_mode ? pkg->binary : pkg->url);
    
    for (int i = 0; i <= 100; i += 10) {
        printf("\r  " GREEN "[");
        for (int j = 0; j < i/2; j++) printf("=");
        for (int j = i/2; j < 50; j++) printf(" ");
        printf("]" NC " %d%%", i);
        fflush(stdout);
        usleep(100000);
    }
    printf("\n");
    
    printf("  " CYAN ">>>" NC " Extracting...\n");
    printf("  " CYAN ">>>" NC " Installing to /usr/local...\n");
    
    if (binary_mode) {
        mark_installed(pkg->name, pkg->version);
        printf("\n  " GREEN "✓ %s %s installed successfully!" NC "\n", pkg->name, pkg->version);
        printf("  " PURPLE "  Use: %s" NC "\n", pkg->name);
    } else {
        mark_installed(pkg->name, pkg->version);
        printf("\n  " GREEN "✓ %s %s compiled and installed!" NC "\n", pkg->name, pkg->version);
    }

    return 0;
}

static int syn_remove(const char *pkg_name) {
    if (!is_installed(pkg_name)) {
        printf("  " YELLOW "[!] %s is not installed." NC "\n", pkg_name);
        return 1;
    }
    printf("  " CYAN ">>>" NC " Removing %s...\n", pkg_name);
    mark_removed(pkg_name);
    printf("  " GREEN "✓ %s removed." NC "\n", pkg_name);
    return 0;
}

static int syn_list(void) {
    ensure_db();
    DIR *d = opendir(PKG_DB_DIR);
    if (!d) {
        printf("  " YELLOW "No packages installed." NC "\n");
        return 0;
    }
    printf("  " CYAN "Installed packages:" NC "\n");
    struct dirent *de;
    int count = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.') continue;
        printf("    " GREEN "●" NC " %s\n", de->d_name);
        count++;
    }
    closedir(d);
    if (count == 0) printf("    (none)\n");
    printf("  " PURPLE "Total: %d packages" NC "\n", count);
    return 0;
}

static int syn_search(const char *query) {
    printf("  " CYAN "Searching for '%s'..." NC "\n\n", query);
    int found = 0;
    for (int i = 0; pkg_db[i].name; i++) {
        if (strstr(pkg_db[i].name, query) || strstr(pkg_db[i].desc, query)) {
            printf("  " GREEN "●" NC " " PURPLE "%-20s" NC " %-8s  %s\n", 
                   pkg_db[i].name, pkg_db[i].version, pkg_db[i].desc);
            found++;
        }
    }
    if (!found) printf("  " YELLOW "No results for '%s'." NC "\n", query);
    else printf("\n  " PURPLE "%d packages matched." NC "\n", found);
    return 0;
}

static int syn_update(void) {
    printf("  " CYAN ">>>" NC " Syncing package database...\n");
    for (int i = 0; i <= 100; i += 10) {
        printf("\r  " GREEN "[");
        for (int j = 0; j < i/2; j++) printf("=");
        for (int j = i/2; j < 50; j++) printf(" ");
        printf("]" NC " %d%%", i);
        fflush(stdout);
        usleep(50000);
    }
    printf("\n  " GREEN "✓ Package database is up to date." NC "\n");
    return 0;
}

static int syn_info(const char *pkg_name) {
    PkgEntry *pkg = find_pkg(pkg_name);
    if (!pkg) {
        printf("  " RED "[!] Package '%s' not found." NC "\n", pkg_name);
        return 1;
    }
    printf("\n");
    printf("  " CYAN "╔════════════════════════════════════════════════════╗" NC "\n");
    printf("  " CYAN "║" NC "  Package:  " GREEN "%-20s" NC "               " CYAN "║" NC "\n", pkg->name);
    printf("  " CYAN "║" NC "  Version:  %-20s               " CYAN "║" NC "\n", pkg->version);
    printf("  " CYAN "║" NC "  Desc:     %-30s" CYAN "║" NC "\n", pkg->desc);
    printf("  " CYAN "║" NC "  Deps:     %-20s               " CYAN "║" NC "\n", pkg->deps);
    printf("  " CYAN "║" NC "  URL:      %-30s" CYAN "║" NC "\n", pkg->url);
    printf("  " CYAN "║" NC "  Installed: %-20s               " CYAN "║" NC "\n", 
           is_installed(pkg_name) ? "Yes" GREEN "●" NC : "No" RED "○" NC);
    printf("  " CYAN "╚════════════════════════════════════════════════════╝" NC "\n");
    return 0;
}

/* ─── Main ─── */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_banner();
        printf("  " YELLOW "Usage:" NC "\n");
        printf("    " CYAN "syn inst <pkg>" NC "     Install a package\n");
        printf("    " CYAN "syn binary <pkg>" NC "   Install a binary package\n");
        printf("    " CYAN "syn remove <pkg>" NC "   Remove a package\n");
        printf("    " CYAN "syn list" NC "            List installed packages\n");
        printf("    " CYAN "syn search <q>" NC "      Search packages\n");
        printf("    " CYAN "syn update" NC "           Update package database\n");
        printf("    " CYAN "syn info <pkg>" NC "      Show package information\n");
        printf("\n");
        printf("  " PURPLE "Examples:" NC "\n");
        printf("    " GREEN "sudo syn inst telegram-desktop" NC "\n");
        printf("    " GREEN "sudo syn binary firefox" NC "\n");
        printf("    " GREEN "syn list" NC "\n");
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "inst") == 0 || strcmp(cmd, "install") == 0) {
        if (argc < 3) { fprintf(stderr, "Usage: syn inst <package>\n"); return 1; }
        return syn_install(argv[2], 0);
    }
    else if (strcmp(cmd, "binary") == 0) {
        if (argc < 3) { fprintf(stderr, "Usage: syn binary <package>\n"); return 1; }
        return syn_install(argv[2], 1);
    }
    else if (strcmp(cmd, "remove") == 0 || strcmp(cmd, "rm") == 0) {
        if (argc < 3) { fprintf(stderr, "Usage: syn remove <package>\n"); return 1; }
        return syn_remove(argv[2]);
    }
    else if (strcmp(cmd, "list") == 0 || strcmp(cmd, "ls") == 0) {
        return syn_list();
    }
    else if (strcmp(cmd, "search") == 0) {
        if (argc < 3) { fprintf(stderr, "Usage: syn search <query>\n"); return 1; }
        return syn_search(argv[2]);
    }
    else if (strcmp(cmd, "update") == 0) {
        return syn_update();
    }
    else if (strcmp(cmd, "info") == 0) {
        if (argc < 3) { fprintf(stderr, "Usage: syn info <package>\n"); return 1; }
        return syn_info(argv[2]);
    }
    else {
        fprintf(stderr, "  " RED "Unknown command: %s" NC "\n", cmd);
        fprintf(stderr, "  " YELLOW "Try: syn inst, syn binary, syn list, syn search, syn update, syn info" NC "\n");
        return 1;
    }
}
