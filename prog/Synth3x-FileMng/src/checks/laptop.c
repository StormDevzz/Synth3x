#define _GNU_SOURCE
#include "laptop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

static int read_sysfs_str(const char *path, char *buf, size_t size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, buf, size - 1);
    close(fd);
    if (n <= 0) return -1;
    buf[n] = '\0';
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == ' ')) buf[--n] = '\0';
    return 0;
}

static int has_battery(void) {
    DIR *d = opendir("/sys/class/power_supply");
    if (!d) return 0;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (strncmp(e->d_name, "BAT", 3) == 0) {
            closedir(d);
            return 1;
        }
    }
    closedir(d);
    return 0;
}

static int chassis_is_portable(void) {
    char buf[16];
    if (read_sysfs_str("/sys/class/dmi/id/chassis_type", buf, sizeof(buf)) < 0)
        return -1;
    int t = atoi(buf);
    switch (t) {
        case 8:  case 9:  case 10:
        case 11: case 14: case 31:
            return 1;
        default:
            return 0;
    }
}

static int has_lid(void) {
    return access("/proc/acpi/button/lid/LID0/state", F_OK) == 0 ||
           access("/proc/acpi/button/lid/LID/state", F_OK) == 0;
}

int hw_is_laptop(void) {
    if (has_battery()) return 1;
    int ch = chassis_is_portable();
    if (ch > 0) return 1;
    if (has_lid()) return 1;
    return 0;
}

void hw_laptop_model(char *buf, size_t size) {
    char vendor[128] = "Unknown";
    char product[128] = "Unknown";
    char version[128] = "";

    read_sysfs_str("/sys/class/dmi/id/sys_vendor", vendor, sizeof(vendor));
    read_sysfs_str("/sys/class/dmi/id/product_name", product, sizeof(product));

    if (read_sysfs_str("/sys/class/dmi/id/product_version", version, sizeof(version)) == 0) {
        snprintf(buf, size, "%s %s (%s)", vendor, product, version);
    } else {
        snprintf(buf, size, "%s %s", vendor, product);
    }
}

const char *hw_laptop_description(void) {
    static char desc[256];
    char model[192];
    hw_laptop_model(model, sizeof(model));

    if (hw_is_laptop()) {
        snprintf(desc, sizeof(desc), "laptop: %s", model);
    } else {
        snprintf(desc, sizeof(desc), "desktop: %s", model);
    }
    return desc;
}

#ifdef STANDALONE
int main(void) {
    printf("%s\n", hw_laptop_description());
    return 0;
}
#endif
