#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("[*] Strict Check: Amnesic RAM & Swap State\n");
    
    FILE *f = fopen("/proc/swaps", "r");
    if (!f) {
        printf("[!] ERROR: Cannot read /proc/swaps\n");
        return 1;
    }
    
    char line[256];
    int lines_count = 0;
    while (fgets(line, sizeof(line), f)) {
        lines_count++;
    }
    fclose(f);
    
    // /proc/swaps always has 1 header line. If it has more, swap is active!
    if (lines_count <= 1) {
        printf("[+] SUCCESS: Swap space is fully DISABLED (Amnesic state verified)\n");
        return 0;
    } else {
        printf("[-] FAILED: Swap is ACTIVE! Threat of physical forensic data leakage!\n");
        return 1;
    }
}
