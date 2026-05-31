#include <stdio.h>
#include <sys/sysinfo.h>

int main() {
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        printf("RAM: Unable to read memory info\n");
        return 1;
    }
    unsigned long total_ram = (si.totalram * si.mem_unit) / (1024 * 1024);
    unsigned long free_ram = (si.freeram * si.mem_unit) / (1024 * 1024);
    unsigned long used_ram = total_ram - free_ram;
    int percent = (total_ram > 0) ? (int)(used_ram * 100 / total_ram) : 0;
    
    printf("RAM: %luMB / %luMB (%d%%)\n", used_ram, total_ram, percent);
    return 0;
}
