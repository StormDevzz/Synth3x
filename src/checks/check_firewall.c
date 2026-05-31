#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("[*] Strict Check: Firewall Redirection Enforcment\n");
    
    // Probing nftables ruleset using a check system call
    int status = system("nft list ruleset | grep -q \"redirect to :9040\"");
    if (status == 0) {
        printf("[+] SUCCESS: nftables transparent NAT redirection is ACTIVE\n");
        return 0;
    } else {
        printf("[-] FAILED: Firewall redirection is INACTIVE or rules not loaded\n");
        return 1;
    }
}
