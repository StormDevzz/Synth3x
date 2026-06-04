/* Synth3x OS — Hardware Detection
 * Combines assembly CPUID routines with Linux /sys /proc filesystem
 * scanning to detect laptop models, touchpads, and other hardware.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#include "hw_detect.h"

/* ─── USB/PCI Vendor lookup table ─── */
typedef struct { uint16_t id; const char *name; } VendorEntry;

static const VendorEntry vendor_table[] = {
    {0x0488, "Acer"},
    {0x059b, "Acer"},
    {0x1022, "AMD"},
    {0x8086, "Intel"},
    {0x10de, "NVIDIA"},
    {0x1002, "AMD/ATI"},
    {0x14e4, "Broadcom"},
    {0x168c, "Qualcomm/Atheros"},
    {0x10ec, "Realtek"},
    {0x8087, "Intel"},
    {0x0bda, "Realtek"},
    {0x04ca, "Lenovo"},
    {0x17aa, "Lenovo"},
    {0x1028, "Dell"},
    {0x103c, "HP"},
    {0x104d, "Sony"},
    {0x10cf, "Fujitsu"},
    {0x144d, "Samsung"},
    {0x13d1, "LG"},
    {0x04f2, "Chicony"},
    {0x06cb, "Synaptics"},
    {0x044e, "ALPS"},
    {0x04f3, "Elan"},
    {0x0000, NULL}
};

const char *hw_vendor_name(uint16_t vendor_id) {
    for (int i = 0; vendor_table[i].name; i++)
        if (vendor_table[i].id == vendor_id)
            return vendor_table[i].name;
    return "Unknown";
}

/* ─── Device class name lookup ─── */
typedef struct { uint16_t class; const char *name; } ClassEntry;

static const ClassEntry class_table[] = {
    {0x0100, "SCSI"},
    {0x0101, "IDE"},
    {0x0106, "SATA"},
    {0x0108, "NVMe"},
    {0x0200, "Ethernet"},
    {0x0280, "WiFi/Network"},
    {0x0300, "VGA"},
    {0x0301, "XGA"},
    {0x0401, "Audio"},
    {0x0403, "Audio"},
    {0x0600, "Host Bridge"},
    {0x0601, "ISA Bridge"},
    {0x0604, "PCI Bridge"},
    {0x0700, "Serial/Modem"},
    {0x0800, "PIC"},
    {0x0801, "DMA"},
    {0x0803, "Timer"},
    {0x0c00, "FireWire"},
    {0x0c03, "USB"},
    {0x0c05, "SMBus"},
    {0x0d00, "IRDA"},
    {0x0e00, "IDE"},
    {0x1100, "Touchpad/Tablet"},
    {0x1180, "Touchscreen"},
    {0x0000, NULL}
};

const char *hw_device_class_name(uint16_t class_code) {
    for (int i = 0; class_table[i].name; i++)
        if (class_table[i].class == class_code)
            return class_table[i].name;
    return "Unknown";
}

/* ─── Read a sysfs file into buf ─── */
static int read_sysfs(const char *path, char *buf, int max_len) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    int n = read(fd, buf, max_len - 1);
    close(fd);
    if (n > 0) {
        buf[n] = '\0';
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
        return 0;
    }
    return -1;
}

/* ─── Touchpad detection via /proc/bus/input/devices ─── */
int hw_has_touchpad(void) {
    int fd = open("/proc/bus/input/devices", O_RDONLY);
    if (fd < 0) return 0;

    char buf[4096];
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';

    /* Look for common touchpad indicators */
    const char *keywords[] = {
        "Touchpad", "touchpad", "Synaptics", "synaptics",
        "ALPS", "alps", "Elan", "elan",
        "TrackPoint", "trackpoint",
        "bcm5974", "BMP", "Finger",
        NULL
    };
    for (int i = 0; keywords[i]; i++) {
        if (strstr(buf, keywords[i]))
            return 1;
    }
    return 0;
}

int hw_has_synaptics(void) {
    int fd = open("/proc/bus/input/devices", O_RDONLY);
    if (fd < 0) return 0;
    char buf[4096];
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = '\0';
    return strstr(buf, "Synaptics") ? 1 : 0;
}

/* ─── DMI vendor detection via /sys/class/dmi/id/ ─── */
int hw_dmi_vendor(char *buf, int max_len) {
    return read_sysfs("/sys/class/dmi/id/sys_vendor", buf, max_len);
}

int hw_dmi_product(char *buf, int max_len) {
    return read_sysfs("/sys/class/dmi/id/product_name", buf, max_len);
}

int hw_is_laptop(void) {
    /* Check for battery */
    DIR *d = opendir("/sys/class/power_supply");
    if (d) {
        struct dirent *de;
        while ((de = readdir(d))) {
            if (strstr(de->d_name, "BAT") || strstr(de->d_name, "bat")) {
                closedir(d);
                return 1;
            }
        }
        closedir(d);
    }
    /* Check for lid switch */
    if (access("/proc/acpi/button/lid", F_OK) == 0 ||
        access("/sys/class/dmi/id/chassis_type", F_OK) == 0) {
        char type[16];
        if (read_sysfs("/sys/class/dmi/id/chassis_type", type, sizeof(type)) == 0) {
            /* 10 = Laptop, 9 = Notebook, 3 = Desktop, 8 = Portable */
            int t = atoi(type);
            if (t == 10 || t == 9 || t == 8)
                return 1;
        }
    }
    return 0;
}

/* ─── Laptop model identification ─── */
static char laptop_name[128] = "Unknown System";

const char *hw_laptop_name(void) {
    char vendor[64] = "", product[64] = "";
    if (hw_dmi_vendor(vendor, sizeof(vendor)) == 0 &&
        hw_dmi_product(product, sizeof(product)) == 0) {
        snprintf(laptop_name, sizeof(laptop_name), "%s %s", vendor, product);
    }
    return laptop_name;
}

/* ─── Auto-configuration ─── */
void hw_auto_configure(void) {
    char vendor[64] = "", product[64] = "";
    hw_dmi_vendor(vendor, sizeof(vendor));
    hw_dmi_product(product, sizeof(product));

    /* Detect CPU */
    char cpu_vendor[16] = "";
    hw_cpuid_vendor(cpu_vendor);
    int has_avx = hw_cpuid_has_avx();
    int has_sse2 = hw_cpuid_has_sse2();

    printf("[HW] CPU: %s | AVX: %s | SSE2: %s\n",
           cpu_vendor, has_avx ? "Yes" : "No", has_sse2 ? "Yes" : "No");

    /* Detect system type */
    if (vendor[0]) {
        printf("[HW] System: %s %s\n", vendor, product);

        /* Load appropriate kernel modules for known vendors */
        if (strstr(vendor, "LENOVO") || strstr(vendor, "IBM")) {
            printf("[HW] Lenovo detected — loading thinkpad_acpi, trackpoint\n");
            system("modprobe thinkpad_acpi 2>/dev/null || true");
            system("modprobe psmouse 2>/dev/null || true");
        } else if (strstr(vendor, "ACER") || strstr(vendor, "Gateway")) {
            printf("[HW] Acer detected — loading acer-wmi, acerhdf\n");
            system("modprobe acer-wmi 2>/dev/null || true");
            system("modprobe acerhdf 2>/dev/null || true");
            system("modprobe psmouse 2>/dev/null || true");
        } else if (strstr(vendor, "DELL")) {
            printf("[HW] Dell detected — loading dell_wmi, dell_laptop\n");
            system("modprobe dell_wmi 2>/dev/null || true");
            system("modprobe dell_laptop 2>/dev/null || true");
        } else if (strstr(vendor, "HP")) {
            printf("[HW] HP detected — loading hp-wmi, hp_accel\n");
            system("modprobe hp-wmi 2>/dev/null || true");
            system("modprobe hp_accel 2>/dev/null || true");
        }
    }

    /* Touchpad support */
    if (hw_has_touchpad()) {
        printf("[HW] Touchpad detected\n");
        /* Load touchpad drivers */
        system("modprobe psmouse 2>/dev/null || true");
        system("modprobe i2c-i801 2>/dev/null || true");
        system("modprobe i2c-hid 2>/dev/null || true");

        if (hw_has_synaptics()) {
            printf("[HW] Synaptics touchpad — loading synapticus_rmi4\n");
            system("modprobe rmi_core 2>/dev/null || true");
            system("modprobe rmi_smbus 2>/dev/null || true");
        }
    } else {
        printf("[HW] No touchpad detected (using mouse)\n");
        system("modprobe psmouse 2>/dev/null || true");
    }

    /* Load WiFi modules using insmod in dependency order */
    system("for d in /lib/modules/*/; do \
             [ -f \"${d}cfg80211.ko\" ] || continue; \
             insmod \"${d}cfg80211.ko\" 2>/dev/null; \
             insmod \"${d}mac80211.ko\" 2>/dev/null; \
             insmod \"${d}iwlwifi.ko\" 2>/dev/null; \
             insmod \"${d}iwlmvm.ko\" 2>/dev/null; \
             insmod \"${d}iwlmld.ko\" 2>/dev/null; \
             insmod \"${d}iwldvm.ko\" 2>/dev/null; \
             insmod \"${d}ath.ko\" 2>/dev/null; \
             insmod \"${d}ath9k_hw.ko\" 2>/dev/null; \
             insmod \"${d}ath9k_common.ko\" 2>/dev/null; \
             insmod \"${d}ath9k.ko\" 2>/dev/null; \
             insmod \"${d}ath10k_core.ko\" 2>/dev/null; \
             insmod \"${d}ath10k_pci.ko\" 2>/dev/null; \
             insmod \"${d}rtl8xxxu.ko\" 2>/dev/null; \
             insmod \"${d}rtw88_core.ko\" 2>/dev/null; \
             insmod \"${d}rtw88_8822ce.ko\" 2>/dev/null; \
             break; done 2>/dev/null || true");

    /* Ethernet / virtual NIC drivers */
    system("for d in /lib/modules/*/; do \
             [ -f \"${d}failover.ko\"    ] && insmod \"${d}failover.ko\"    2>/dev/null; \
             [ -f \"${d}net_failover.ko\" ] && insmod \"${d}net_failover.ko\" 2>/dev/null; \
             [ -f \"${d}virtio_net.ko\"   ] && insmod \"${d}virtio_net.ko\"   2>/dev/null; \
             [ -f \"${d}e1000.ko\"        ] && insmod \"${d}e1000.ko\"        2>/dev/null; \
             break; done 2>/dev/null || true");
    system("modprobe e1000e 2>/dev/null || true");
    system("modprobe r8169 2>/dev/null || true");

    /* Load sound modules */
    system("modprobe snd_hda_intel 2>/dev/null || true");
    system("modprobe snd_hda_codec 2>/dev/null || true");
    system("modprobe snd_hda_core 2>/dev/null || true");
    system("modprobe snd_pcm 2>/dev/null || true");

    printf("[HW] Hardware auto-configuration complete\n");
}
