/* Synth3x-Anon — Driver: Mouse & Touchpad Check */
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
    printf("\n  === Mouse & Touchpad Driver Check ===\n\n");

    /* 1. Check /dev/input/mice */
    int has_mice = (access("/dev/input/mice", F_OK) == 0);
    printf("  /dev/input/mice:  %s\n", has_mice ? GREEN "EXISTS" NC : RED "NOT FOUND" NC);

    /* 2. Check event devices for mouse/touchpad */
    int found_mouse = 0, found_touchpad = 0;
    DIR *d = opendir("/dev/input");
    if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
            if (strncmp(de->d_name, "event", 5) != 0) continue;
            char path[64];
            snprintf(path, sizeof(path), "/dev/input/%s", de->d_name);
            int fd = open(path, O_RDONLY);
            if (fd < 0) continue;

            unsigned char ev_bits[EV_MAX/8 + 1] = {0};
            if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) >= 0) {
                if (ev_bits[EV_KEY/8] & (1 << (EV_KEY % 8))) {
                    unsigned char key_bits[KEY_CNT/8 + 1] = {0};
                    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
                    if ((key_bits[BTN_MOUSE/8] & (1 << (BTN_MOUSE % 8))) ||
                        (key_bits[BTN_LEFT/8] & (1 << (BTN_LEFT % 8)))) {
                        found_mouse = 1;
                        char name[128] = "unknown";
                        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
                        if (strstr(name, "Touch") || strstr(name, "touch") ||
                            strstr(name, "pad") || strstr(name, "Synaptics") ||
                            strstr(name, "Elan")) {
                            found_touchpad = 1;
                            printf("  Touchpad:         %s%s%s (%s)\n", GREEN, name, NC, de->d_name);
                        } else {
                            printf("  Mouse:            %s%s%s (%s)\n", GREEN, name, NC, de->d_name);
                        }
                    }
                }
            }
            close(fd);
        }
        closedir(d);
    }

    if (!found_mouse)
        printf("  Pointing device:  %sNOT DETECTED%s\n", RED, NC);

    /* 3. Check psmouse module */
    FILE *mod = fopen("/proc/modules", "r");
    if (mod) {
        char line[256];
        while (fgets(line, sizeof(line), mod)) {
            if (strstr(line, "psmouse"))
                printf("  psmouse module:   %sloaded%s\n", GREEN, NC);
            if (strstr(line, "elan_i2c"))
                printf("  elan_i2c module:  %sloaded%s (touchpad)\n", GREEN, NC);
        }
        fclose(mod);
    }

    printf("\n  Recommendation:\n");
    if (!found_mouse && !found_touchpad)
        printf("    No pointing device. Try: modprobe psmouse\n");
    else if (found_touchpad)
        printf("    Touchpad OK. Multi-touch gestures supported.\n");
    else
        printf("    Mouse OK.\n");

    printf("\n  === Check complete ===\n");
    return found_mouse ? 0 : 1;
}
