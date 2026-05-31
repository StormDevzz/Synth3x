#include <stdio.h>
#include <string.h>

int main() {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    char cpu_name[128] = "QEMU Virtual CPU";
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    colon++;
                    while (*colon == ' ') colon++;
                    char *nl = strchr(colon, '\n');
                    if (nl) *nl = '\0';
                    strncpy(cpu_name, colon, sizeof(cpu_name) - 1);
                    cpu_name[sizeof(cpu_name) - 1] = '\0';
                    break;
                }
            }
        }
        fclose(fp);
    }
    printf("CPU: %s\n", cpu_name);
    
    FILE *ufp = popen("uname -m", "r");
    char arch[32] = "x86_64";
    if (ufp) {
        if (fgets(arch, sizeof(arch), ufp)) {
            char *nl = strchr(arch, '\n');
            if (nl) *nl = '\0';
        }
        pclose(ufp);
    }
    printf("ARCH: %s\n", arch);
    
    return 0;
}
