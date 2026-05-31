#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

int main() {
    printf("[*] Strict Check: MAC Address Randomization\n");
    
    DIR *d = opendir("/sys/class/net");
    if (!d) {
        printf("[!] ERROR: Cannot read network classes\n");
        return 1;
    }
    
    struct dirent *de;
    int failed = 0;
    while ((de = readdir(d))) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, "lo") == 0)
            continue;
        
        char path[128];
        snprintf(path, sizeof(path), "/sys/class/net/%s/address", de->d_name);
        FILE *f = fopen(path, "r");
        if (f) {
            char mac[32];
            if (fgets(mac, sizeof(mac), f)) {
                // Check if locally administered bit is set in first byte (e.g. 02:...)
                if (mac[0] == '0' && mac[1] == '2') {
                    printf("[+] SUCCESS: Interface %s has spoofed MAC (%s)\n", de->d_name, strtok(mac, "\n"));
                } else {
                    printf("[-] WARNING: Interface %s has non-spoofed MAC (%s)\n", de->d_name, strtok(mac, "\n"));
                    failed = 1;
                }
            }
            fclose(f);
        }
    }
    closedir(d);
    
    return failed;
}
