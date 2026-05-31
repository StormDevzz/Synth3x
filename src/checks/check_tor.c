#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

int main() {
    printf("[*] Strict Check: Tor Daemon Integrity\n");
    
    // Check if Tor is running via /proc
    DIR *d = opendir("/proc");
    if (!d) {
        printf("[!] ERROR: Cannot read /proc filesystem\n");
        return 1;
    }
    
    struct dirent *de;
    int tor_found = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] >= '0' && de->d_name[0] <= '9') {
            char path[128];
            snprintf(path, sizeof(path), "/proc/%s/comm", de->d_name);
            FILE *f = fopen(path, "r");
            if (f) {
                char comm[32];
                if (fgets(comm, sizeof(comm), f)) {
                    if (strncmp(comm, "tor", 3) == 0) {
                        tor_found = 1;
                        fclose(f);
                        break;
                    }
                }
                fclose(f);
            }
        }
    }
    closedir(d);
    
    if (tor_found) {
        printf("[+] SUCCESS: Tor Daemon is running\n");
        return 0;
    } else {
        printf("[-] FAILED: Tor Daemon is NOT running\n");
        return 1;
    }
}
