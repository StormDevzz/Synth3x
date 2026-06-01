/* Synth3x-Anon — Driver: Sound Devices Check */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define NC "\033[0m"

static int check_dev(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

static int check_sysfs(const char *path) {
    return (access(path, F_OK) == 0);
}

int main(void) {
    printf("\n  === Sound Driver Check ===\n\n");

    /* 1. Check ALSA */
    int has_alsa = check_dev("/dev/snd/controlC0") || check_dev("/dev/snd/seq");
    printf("  ALSA devices:       %s\n", has_alsa ? GREEN "YES" NC : RED "NO" NC);

    /* 2. Check /proc/asound */
    int has_proc_asound = check_sysfs("/proc/asound/cards");
    printf("  /proc/asound:       %s\n", has_proc_asound ? GREEN "YES" NC : RED "NO" NC);

    /* 3. Check PCI audio controllers */
    DIR *d = opendir("/sys/bus/pci/devices");
    int has_pci_audio = 0;
    if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
            if (de->d_name[0] == '.') continue;
            char path[256];
            snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/class", de->d_name);
            FILE *f = fopen(path, "r");
            if (f) {
                char cls[16];
                if (fgets(cls, sizeof(cls), f) && strncmp(cls, "0x04", 4) == 0)
                    has_pci_audio = 1;
                fclose(f);
                if (has_pci_audio) break;
            }
        }
        closedir(d);
    }
    printf("  PCI audio adapter:  %s\n", has_pci_audio ? GREEN "DETECTED" NC : YELLOW "none" NC);

    /* 4. Check loaded modules */
    FILE *mod = fopen("/proc/modules", "r");
    int has_snd = 0, has_hda = 0, has_usb = 0;
    if (mod) {
        char line[256];
        while (fgets(line, sizeof(line), mod)) {
            if (strstr(line, "snd_hda_intel")) has_hda = 1;
            if (strstr(line, "snd_usb_audio")) has_usb = 1;
            if (strstr(line, "snd_")) has_snd = 1;
        }
        fclose(mod);
    }
    printf("  snd_hda_intel:      %s\n", has_hda ? GREEN "loaded" NC : RED "not loaded" NC);
    printf("  snd_usb_audio:      %s\n", has_usb ? GREEN "loaded" NC : YELLOW "not loaded" NC);

    /* Recommendation */
    printf("\n  Recommendation:\n");
    if (!has_pci_audio)
        printf("    No audio hardware detected, none needed.\n");
    else if (!has_hda)
        printf("    Run: modprobe snd_hda_intel\n");
    if (!has_usb && has_pci_audio)
        printf("    For USB audio: modprobe snd_usb_audio\n");

    printf("\n  === Check complete ===\n");
    return 0;
}
