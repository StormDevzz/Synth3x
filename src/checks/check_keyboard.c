/* Synth3x-Anon — Driver: Keyboard Check */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/input.h>

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define NC "\033[0m"

int main(void) {
    printf("\n  === Keyboard Driver Check ===\n\n");

    /* 1. Check /dev/input/event* for KEY capability */
    int found_kbd = 0, found_ev = 0;
    DIR *d = opendir("/dev/input");
    if (!d) {
        printf("  /dev/input: %sNOT FOUND%s (no input subsystem)\n", RED, NC);
        return 1;
    }

    struct dirent *de;
    while ((de = readdir(d))) {
        if (strncmp(de->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", de->d_name);
        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;
        found_ev++;

        unsigned char ev_bits[EV_MAX/8 + 1] = {0};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) >= 0) {
            if (ev_bits[EV_KEY/8] & (1 << (EV_KEY % 8))) {
                unsigned char key_bits[KEY_CNT/8 + 1] = {0};
                if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) >= 0) {
                    if (key_bits[KEY_A/8] & (1 << (KEY_A % 8))) {
                        found_kbd = 1;
                        char name[128] = "unknown";
                        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
                        printf("  Keyboard:       %s%s%s (%s)\n", GREEN, name, NC, de->d_name);
                    }
                }
            }
        }
        close(fd);
    }
    closedir(d);

    if (!found_ev)
        printf("  Input devices:  %sNONE%s\n", RED, NC);
    if (!found_kbd)
        printf("  Keyboard:       %sNOT DETECTED%s\n", RED, NC);

    /* 2. Check /proc/bus/input/devices */
    FILE *proc = fopen("/proc/bus/input/devices", "r");
    if (proc) {
        char line[256];
        int kbd_proc = 0;
        while (fgets(line, sizeof(line), proc)) {
            if (strstr(line, "keyboard") || strstr(line, "Keyboard") ||
                strstr(line, "AT Translated") || strstr(line, "kbd"))
                kbd_proc = 1;
        }
        fclose(proc);
        if (kbd_proc && !found_kbd)
            printf("  (detected in /proc/bus/input/devices but no event device)\n");
    }

    /* 3. Check atkbd module */
    FILE *mod = fopen("/proc/modules", "r");
    if (mod) {
        char line[256];
        while (fgets(line, sizeof(line), mod)) {
            if (strstr(line, "atkbd")) {
                printf("  atkbd module:   %sloaded%s\n", GREEN, NC);
                break;
            }
        }
        fclose(mod);
    }

    printf("\n  Recommendation:\n");
    if (!found_kbd)
        printf("    Keyboard not detected. Check VM USB/PS2 settings.\n");
    else
        printf("    Keyboard OK. Type here to test: ");

    printf("\n  === Check complete ===\n");
    return found_kbd ? 0 : 1;
}
