/* Synth3x OS — init process
 * First userspace process.
 * Starts the selected desktop environment.
 * Pure C, no external dependencies beyond libc.
 */

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

#define SYNTH3X_DE  "/usr/bin/synth3x"
#define XFCE_SESSION "/usr/bin/startxfce4"
#define SHELL       "/bin/sh"

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static void setup_system(void) {
    system("mount -t proc proc /proc 2>/dev/null");
    system("mount -t sysfs sysfs /sys 2>/dev/null");
    system("mount -t devtmpfs devtmpfs /dev 2>/dev/null");
    system("mount -t tmpfs tmpfs /tmp 2>/dev/null");
    system("mkdir -p /dev/pts 2>/dev/null");
    system("mount -t devpts devpts /dev/pts 2>/dev/null");

    int fd = open("/proc/sys/kernel/hostname", O_WRONLY);
    if (fd >= 0) {
        write(fd, "synth3x\n", 8);
        close(fd);
    }
    system("mount -t tmpfs -o size=10M tmpfs /var/log 2>/dev/null");
    system("ip link set lo up 2>/dev/null");
}

static void print_banner(void) {
    printf("\n");
    printf("  \033[1;32m╔══════════════════════════════════════════╗\033[0m\n");
    printf("  \033[1;32m║        S Y N T H 3 X   O S             ║\033[0m\n");
    printf("  \033[1;32m║   Desktop Environment Selector v0.2     ║\033[0m\n");
    printf("  \033[1;32m╚══════════════════════════════════════════╝\033[0m\n");
    printf("\n");
}

static void print_env_status(void) {
    printf("  Available environments:\n");
    if (file_exists(SYNTH3X_DE))
        printf("    \033[1;32m[1] Synth3x DE\033[0m  — built-in (C framebuffer compositor)\n");
    else
        printf("    \033[1;31m[1] Synth3x DE\033[0m  — NOT AVAILABLE\n");

    if (file_exists(XFCE_SESSION))
        printf("    \033[1;32m[2] Xfce\033[0m        — installed\n");
    else
        printf("    \033[1;33m[2] Xfce\033[0m        — not in live environment\n");

    printf("    \033[1;32m[s] Shell\033[0m      — command line\n");
    printf("    [r] Reboot\n");
    printf("    [h] Halt\n");
    printf("\n");
    printf("  Choice: ");
    fflush(stdout);
}

static void run_shell(void) {
    printf("init: starting shell\n");
    execl(SHELL, "sh", NULL);
    printf("init: failed to start shell!\n");
}

int main(int argc, char *argv[]) {
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    setup_system();
    print_banner();

    /* Check if we have any DE available */
    int has_synth3x = file_exists(SYNTH3X_DE);
    int has_xfce = file_exists(XFCE_SESSION);

    /* If only one is available, auto-select it */
    if (has_synth3x && !has_xfce) {
        printf("  Only Synth3x DE is available. Auto-starting...\n\n");
        goto start_synth3x;
    }
    if (!has_synth3x && has_xfce) {
        printf("  Only Xfce is available. Auto-starting...\n\n");
        goto start_xfce;
    }

    /* Interactive selection */
    char env = 0;
    while (!env) {
        print_env_status();

        /* Check kernel param first */
        FILE *cmdline = fopen("/proc/cmdline", "r");
        if (cmdline) {
            char buf[512] = {0};
            size_t n = fread(buf, 1, sizeof(buf) - 1, cmdline);
            fclose(cmdline);
            buf[n] = 0;
            if (strstr(buf, "synth3x.env=xfce"))
                env = '2';
            else if (strstr(buf, "synth3x.env=shell"))
                env = 's';
            else if (strstr(buf, "synth3x.env=synth3x"))
                env = '1';
        }

        if (!env)
            env = getchar();
        /* consume rest of line */
        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        switch (env) {
            case '1': goto start_synth3x;
            case '2': goto start_xfce;
            case 's': case 'S': goto start_shell;
            case 'r': case 'R':
                printf("  Rebooting...\n");
                system("reboot -f 2>/dev/null");
                system("echo b > /proc/sysrq-trigger 2>/dev/null");
                break;
            case 'h': case 'H':
                printf("  Halting...\n");
                system("poweroff -f 2>/dev/null");
                system("echo o > /proc/sysrq-trigger 2>/dev/null");
                break;
            default:
                printf("\n  Invalid choice. Try again.\n");
                env = 0;
        }
    }

start_synth3x:
    printf("  Starting Synth3x DE...\n\n");
    execl(SYNTH3X_DE, "synth3x", NULL);
    printf("init: failed to exec Synth3x DE!\n");
    goto start_shell;

start_xfce:
    if (!has_xfce) {
        printf("  Xfce is not available in this environment.\n");
        printf("  Falling back to shell.\n\n");
        goto start_shell;
    }
    printf("  Starting Xfce...\n\n");
    execl(XFCE_SESSION, "startxfce4", NULL);
    printf("init: failed to exec Xfce!\n");
    goto start_shell;

start_shell:
    run_shell();
    printf("init: shell exited, halting.\n");
    return 0;
}
