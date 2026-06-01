/* Synth3x-Anon — Driver: Display / GPU Check */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define NC "\033[0m"

int main(void) {
    printf("\n  === Display / GPU Driver Check ===\n\n");

    /* 1. Check /dev/fb0 */
    int has_fb = (access("/dev/fb0", F_OK) == 0);
    printf("  Framebuffer:     %s\n", has_fb ? GREEN "/dev/fb0" NC : RED "NOT FOUND" NC);

    if (has_fb) {
        int fd = open("/dev/fb0", O_RDONLY);
        if (fd >= 0) {
            struct fb_var_screeninfo vi;
            struct fb_fix_screeninfo fix;
            if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) == 0 &&
                ioctl(fd, FBIOGET_FSCREENINFO, &fix) == 0) {
                printf("  Resolution:      %s%dx%d %dbpp%s\n", GREEN, vi.xres, vi.yres, vi.bits_per_pixel, NC);
                printf("  Stride:          %d bytes\n", fix.line_length);
                printf("  Framebuffer:     %lu bytes\n", (unsigned long)fix.smem_len);
                printf("  Accel:           %s\n", fix.accel ? YELLOW "hardware" NC : "none (software)");
            }
            close(fd);
        }
    }

    /* 2. Check PCI GPU devices */
    DIR *d = opendir("/sys/bus/pci/devices");
    int has_gpu = 0;
    if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
            if (de->d_name[0] == '.') continue;
            char path[256];
            snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/class", de->d_name);
            FILE *f = fopen(path, "r");
            if (f) {
                char cls[16];
                if (fgets(cls, sizeof(cls), f) && strncmp(cls, "0x03", 4) == 0) {
                    has_gpu = 1;
                    char name[256] = "unknown";
                    FILE *nf = fopen("/sys/bus/pci/devices/%s/name", "r");
                    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/device", de->d_name);
                    FILE *df = fopen(path, "r");
                    if (df) {
                        char devid[16];
                        if (fgets(devid, sizeof(devid), df)) {
                            char *nl = strchr(devid, '\n');
                            if (nl) *nl = 0;
                            printf("  PCI GPU:         %s0x%s%s\n", GREEN, devid, NC);
                        }
                        fclose(df);
                    }
                    if (nf) { fgets(name, sizeof(name), nf); fclose(nf); }
                    printf("                   %s\n", name);
                }
                fclose(f);
            }
        }
        closedir(d);
    }

    if (!has_gpu && !has_fb)
        printf("  GPU:             %sNONE DETECTED%s\n", RED, NC);

    /* 3. Check loaded GPU modules */
    FILE *mod = fopen("/proc/modules", "r");
    if (mod) {
        char line[256];
        while (fgets(line, sizeof(line), mod)) {
            if (strstr(line, "bochs") || strstr(line, "i915") || strstr(line, "nouveau") ||
                strstr(line, "amdgpu") || strstr(line, "virtio_gpu") || strstr(line, "cirrus"))
                printf("  GPU module:      %s%sloaded%s", GREEN, line, NC);
        }
        fclose(mod);
    }

    /* 4. Check backlight */
    int has_backlight = (access("/sys/class/backlight", F_OK) == 0);
    printf("  Backlight:       %s\n", has_backlight ? GREEN "available" NC : YELLOW "not available" NC);

    printf("\n  Recommendation:\n");
    if (!has_fb)
        printf("    No framebuffer. Load GPU driver: insmod /lib/modules/bochs.ko\n");
    else
        printf("    Display OK. Synth3x DE will use framebuffer directly.\n");

    printf("\n  === Check complete ===\n");
    return has_fb ? 0 : 1;
}
