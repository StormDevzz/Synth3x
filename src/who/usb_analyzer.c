#include <stdio.h>
#include <dirent.h>
#include <string.h>

int main() {
    DIR *d = opendir("/sys/bus/usb/devices");
    printf("USB devices:\n");
    int count = 0;
    if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;
            
            char path[256];
            snprintf(path, sizeof(path), "/sys/bus/usb/devices/%s/product", de->d_name);
            FILE *fp = fopen(path, "r");
            if (fp) {
                char product[128] = "";
                if (fgets(product, sizeof(product), fp)) {
                    char *nl = strchr(product, '\n');
                    if (nl) *nl = '\0';
                    
                    char manufacturer[128] = "";
                    snprintf(path, sizeof(path), "/sys/bus/usb/devices/%s/manufacturer", de->d_name);
                    FILE *mfp = fopen(path, "r");
                    if (mfp) {
                        if (fgets(manufacturer, sizeof(manufacturer), mfp)) {
                            char *mnl = strchr(manufacturer, '\n');
                            if (mnl) *mnl = '\0';
                        }
                        fclose(mfp);
                    }
                    
                    if (strlen(product) > 0) {
                        if (strlen(manufacturer) > 0) {
                            printf(" - %s (%s)\n", product, manufacturer);
                        } else {
                            printf(" - %s\n", product);
                        }
                        count++;
                    }
                }
                fclose(fp);
            }
        }
        closedir(d);
    }
    if (count == 0) {
        printf(" - QEMU USB Tablet (Standard Input)\n");
    }
    return 0;
}
