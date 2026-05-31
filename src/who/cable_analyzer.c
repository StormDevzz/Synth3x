#include <stdio.h>
#include <dirent.h>
#include <string.h>

int main() {
    DIR *d = opendir("/sys/class/net");
    printf("Net cables:\n");
    int count = 0;
    if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, "lo") == 0)
                continue;
            
            char path[256];
            snprintf(path, sizeof(path), "/sys/class/net/%s/carrier", de->d_name);
            FILE *fp = fopen(path, "r");
            int carrier = 0;
            if (fp) {
                if (fscanf(fp, "%d", &carrier) != 1) {
                    carrier = 0;
                }
                fclose(fp);
            }
            
            printf(" - %s: %s\n", de->d_name, carrier ? "LINK ACTIVE (CONNECTED)" : "LINK DOWN (DISCONNECTED)");
            count++;
        }
        closedir(d);
    }
    if (count == 0) {
        printf(" - No network interfaces detected.\n");
    }
    return 0;
}
