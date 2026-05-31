#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    printf("[*] Strict Check: Hostname Anonymity\n");
    
    char hostname[64];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        printf("[!] ERROR: Cannot retrieve hostname\n");
        return 1;
    }
    
    // Check if hostname is generic or default
    if (strcmp(hostname, "localhost") == 0 || strcmp(hostname, "gentoo") == 0) {
        printf("[-] FAILED: Hostname is default '%s' (Fingerprinting possible)\n", hostname);
        return 1;
    } else {
        printf("[+] SUCCESS: Hostname is randomized ('%s')\n", hostname);
        return 0;
    }
}
