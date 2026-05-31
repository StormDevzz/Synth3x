#include <stdio.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <string.h>

int main() {
    struct statvfs vfs;
    if (statvfs("/", &vfs) == 0) {
        unsigned long total = (vfs.f_blocks * vfs.f_frsize) / (1024 * 1024);
        unsigned long free = (vfs.f_bfree * vfs.f_frsize) / (1024 * 1024);
        unsigned long used = total - free;
        int percent = (total > 0) ? (int)(used * 100 / total) : 0;
        printf("DISK space: %luMB/%luMB (%d%%)\n", used, total, percent);
    } else {
        printf("DISK space: Unknown\n");
    }

    DIR *d = opendir("/sys/block");
    if (d) {
        struct dirent *de;
        printf("DISK list: ");
        int count = 0;
        while ((de = readdir(d))) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || 
                strncmp(de->d_name, "loop", 4) == 0 || strncmp(de->d_name, "ram", 3) == 0)
                continue;
            if (count > 0) printf(", ");
            printf("%s", de->d_name);
            count++;
        }
        if (count == 0) printf("None");
        printf("\n");
        closedir(d);
    }
    return 0;
}
