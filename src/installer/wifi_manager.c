/* Synth3x OS v0.8.1 — Installer WiFi Manager (C)
 * Handles WiFi network scanning, connecting, and disconnecting.
 * Uses wpa_supplicant or iwctl depending on what's available.
 * Runs as a helper called by the Rust installer.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>

#define CYAN    "\033[0;36m"
#define PURPLE  "\033[0;35m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[1;33m"
#define RED     "\033[0;31m"
#define DIM     "\033[0;90m"
#define BOLD    "\033[1m"
#define NC      "\033[0m"

#define MAX_SSID_LEN 64
#define MAX_PASS_LEN 128

/* ─── Detect available WiFi tools ─── */
typedef enum {
    WIFI_TOOL_NONE,
    WIFI_TOOL_IWCTL,
    WIFI_TOOL_WPA_SUPPLICANT,
    WIFI_TOOL_NMCLI
} wifi_tool_t;

static wifi_tool_t detect_wifi_tool(void) {
    if (system("which iwctl >/dev/null 2>&1") == 0)
        return WIFI_TOOL_IWCTL;
    if (system("which nmcli >/dev/null 2>&1") == 0)
        return WIFI_TOOL_NMCLI;
    if (system("which wpa_supplicant >/dev/null 2>&1") == 0)
        return WIFI_TOOL_WPA_SUPPLICANT;
    return WIFI_TOOL_NONE;
}

/* ─── List WiFi interfaces ─── */
static int list_interfaces(char interfaces[][16], int max_count) {
    DIR *d = opendir("/sys/class/net");
    if (!d) return 0;

    int count = 0;
    struct dirent *de;
    while ((de = readdir(d)) && count < max_count) {
        if (de->d_name[0] == '.') continue;
        char path[256];
        snprintf(path, sizeof(path), "/sys/class/net/%s/wireless", de->d_name);
        if (access(path, F_OK) == 0) {
            strncpy(interfaces[count], de->d_name, 15);
            interfaces[count][15] = '\0';
            count++;
        }
    }
    closedir(d);
    return count;
}

/* ─── Scan WiFi networks using iwctl ─── */
static int scan_networks_iwctl(const char *iface) {
    printf("  %sScanning WiFi networks (iwctl)...%s\n", CYAN, NC);

    /* Start wpa_supplicant if needed */
    system("wpa_supplicant -B -i wlan0 -c /dev/null >/dev/null 2>&1");
    sleep(1);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "iwctl station %s scan 2>/dev/null", iface);
    system(cmd);
    sleep(2);

    snprintf(cmd, sizeof(cmd), "iwctl station %s get-networks 2>/dev/null", iface);
    printf("\n  %sAvailable networks:%s\n", BOLD, NC);
    printf("  %s──────────────────────────────────────%s\n", DIM, NC);
    int ret = system(cmd);
    printf("  %s──────────────────────────────────────%s\n", DIM, NC);

    return ret == 0 ? 0 : -1;
}

/* ─── Scan WiFi networks using iwlist ─── */
static int scan_networks_iwlist(const char *iface) {
    printf("  %sScanning WiFi networks (iwlist)...%s\n", CYAN, NC);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "iwlist %s scan 2>/dev/null | grep -E 'ESSID|Signal|Encryption'", iface);
    printf("\n  %sAvailable networks:%s\n", BOLD, NC);
    printf("  %s──────────────────────────────────────%s\n", DIM, NC);
    int ret = system(cmd);
    printf("  %s──────────────────────────────────────%s\n", DIM, NC);
    return ret == 0 ? 0 : -1;
}

/* ─── Connect to WiFi using iwctl ─── */
static int connect_iwctl(const char *iface, const char *ssid, const char *password) {
    char cmd[512];

    if (password && strlen(password) > 0) {
        printf("  %sConnecting to '%s' with password...%s\n", CYAN, ssid, NC);
        snprintf(cmd, sizeof(cmd),
                 "iwctl station %s connect '%s' --passphrase '%s' 2>&1",
                 iface, ssid, password);
    } else {
        printf("  %sConnecting to '%s' (open network)...%s\n", CYAN, ssid, NC);
        snprintf(cmd, sizeof(cmd),
                 "iwctl station %s connect '%s' 2>&1",
                 iface, ssid);
    }

    int ret = system(cmd);
    if (ret == 0) {
        printf("  %s[OK] Connected to %s'%s'%s%s\n", GREEN, BOLD, ssid, GREEN, NC);
        /* Run DHCP */
        printf("  %sObtaining IP address...%s\n", CYAN, NC);
        char dhcp_cmd[256];
        snprintf(dhcp_cmd, sizeof(dhcp_cmd), "dhcpcd %s 2>/dev/null || udhcpc -i %s -b -q", iface, iface);
        system(dhcp_cmd);
        sleep(2);
    } else {
        printf("  %s[FAIL] Could not connect to '%s'%s\n", RED, ssid, NC);
    }
    return ret;
}

/* ─── Connect using wpa_supplicant directly ─── */
static int connect_wpa(const char *iface, const char *ssid, const char *password) {
    printf("  %sConnecting via wpa_supplicant...%s\n", CYAN, NC);

    /* Kill existing wpa_supplicant */
    system("killall wpa_supplicant 2>/dev/null");
    sleep(1);

    /* Create wpa_supplicant config */
    FILE *f = fopen("/tmp/wpa_supplicant.conf", "w");
    if (!f) {
        printf("  %s[ERROR] Cannot create wpa config%s\n", RED, NC);
        return -1;
    }
    fprintf(f, "ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev\n");
    fprintf(f, "update_config=1\n");
    fprintf(f, "country=00\n\n");
    fprintf(f, "network={\n");
    fprintf(f, "    ssid=\"%s\"\n", ssid);
    if (password && strlen(password) > 0) {
        fprintf(f, "    psk=\"%s\"\n", password);
    } else {
        fprintf(f, "    key_mgmt=NONE\n");
    }
    fprintf(f, "}\n");
    fclose(f);

    /* Start wpa_supplicant */
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "wpa_supplicant -B -i %s -c /tmp/wpa_supplicant.conf -D nl80211,wext 2>&1",
             iface);
    int ret = system(cmd);
    sleep(3);

    if (ret == 0) {
        printf("  %s[OK] wpa_supplicant started, obtaining IP...%s\n", GREEN, NC);
        char dhcp_cmd[256];
        snprintf(dhcp_cmd, sizeof(dhcp_cmd), "dhcpcd %s 2>/dev/null || udhcpc -i %s -b -q", iface, iface);
        system(dhcp_cmd);
        sleep(2);
    } else {
        printf("  %s[FAIL] wpa_supplicant failed%s\n", RED, NC);
    }
    return ret;
}

/* ─── Connect using nmcli ─── */
static int connect_nmcli(const char *iface, const char *ssid, const char *password) {
    printf("  %sConnecting via nmcli...%s\n", CYAN, NC);
    char cmd[512];

    if (password && strlen(password) > 0) {
        snprintf(cmd, sizeof(cmd),
                 "nmcli device wifi connect '%s' password '%s' ifname %s 2>&1",
                 ssid, password, iface);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "nmcli device wifi connect '%s' ifname %s 2>&1",
                 ssid, iface);
    }

    int ret = system(cmd);
    if (ret == 0) {
        printf("  %s[OK] Connected to %s'%s'%s%s\n", GREEN, BOLD, ssid, GREEN, NC);
    } else {
        printf("  %s[FAIL] Could not connect to '%s'%s\n", RED, ssid, NC);
    }
    return ret;
}

/* ─── Main WiFi connect function ─── */
int wifi_connect(const char *ssid, const char *password) {
    char interfaces[4][16];
    int iface_count = list_interfaces(interfaces, 4);

    if (iface_count == 0) {
        printf("  %s[ERROR] No WiFi interface found!%s\n", RED, NC);
        printf("  %s  Ensure WiFi hardware is present and modules are loaded.%s\n", YELLOW, NC);
        return -1;
    }

    printf("  %sWiFi interfaces found:%s ", CYAN, NC);
    for (int i = 0; i < iface_count; i++)
        printf("%s%s%s ", BOLD, interfaces[i], NC);
    printf("\n");

    wifi_tool_t tool = detect_wifi_tool();
    const char *iface = interfaces[0]; /* Use first WiFi interface */

    switch (tool) {
        case WIFI_TOOL_IWCTL:
            return connect_iwctl(iface, ssid, password);
        case WIFI_TOOL_NMCLI:
            return connect_nmcli(iface, ssid, password);
        case WIFI_TOOL_WPA_SUPPLICANT:
            return connect_wpa(iface, ssid, password);
        default:
            printf("  %s[ERROR] No WiFi tool found!%s\n", RED, NC);
            printf("  %s  Install iwctl, nmcli, or wpa_supplicant.%s\n", YELLOW, NC);
            return -1;
    }
}

/* ─── Scan and display available networks ─── */
int wifi_scan(void) {
    char interfaces[4][16];
    int iface_count = list_interfaces(interfaces, 4);

    if (iface_count == 0) {
        printf("  %s[ERROR] No WiFi interface found!%s\n", RED, NC);
        return -1;
    }

    wifi_tool_t tool = detect_wifi_tool();
    const char *iface = interfaces[0];

    switch (tool) {
        case WIFI_TOOL_IWCTL:
            return scan_networks_iwctl(iface);
        default:
            return scan_networks_iwlist(iface);
    }
}

/* ─── Disconnect from WiFi ─── */
int wifi_disconnect(void) {
    wifi_tool_t tool = detect_wifi_tool();
    switch (tool) {
        case WIFI_TOOL_IWCTL:
            system("iwctl station wlan0 disconnect 2>/dev/null");
            break;
        case WIFI_TOOL_NMCLI:
            system("nmcli device disconnect wlan0 2>/dev/null");
            break;
        default:
            system("killall wpa_supplicant 2>/dev/null");
            break;
    }
    printf("  %s[OK] WiFi disconnected%s\n", GREEN, NC);
    return 0;
}

/* ─── Check if connected to internet ─── */
int wifi_check_connection(void) {
    if (system("ping -c 1 -W 3 1.1.1.1 >/dev/null 2>&1") == 0) {
        printf("  %s[OK] Internet connection available%s\n", GREEN, NC);
        return 1;
    }
    if (system("ping -c 1 -W 3 8.8.8.8 >/dev/null 2>&1") == 0) {
        printf("  %s[OK] Internet connection available%s\n", GREEN, NC);
        return 1;
    }
    printf("  %s[WARN] No internet connection%s\n", YELLOW, NC);
    return 0;
}

#ifdef STANDALONE
int main(int argc, char *argv[]) {
    printf("\n  %s═══════════════════════════════════════════════════%s\n", PURPLE, NC);
    printf("  %s  Synth3x Installer — WiFi Manager v0.8.1%s\n", CYAN, NC);
    printf("  %s═══════════════════════════════════════════════════%s\n\n", PURPLE, NC);

    if (argc < 2) {
        printf("  Usage:\n");
        printf("    %swifi-connect <SSID> <password>%s  — Connect to WiFi\n", CYAN, NC);
        printf("    %swifi-scan%s                       — Scan networks\n", CYAN, NC);
        printf("    %swifi-disconnect%s                 — Disconnect\n", CYAN, NC);
        printf("    %swifi-check%s                      — Check connection\n", CYAN, NC);
        return 1;
    }

    if (strcmp(argv[1], "wifi-connect") == 0) {
        if (argc < 3) {
            printf("  %s[ERROR] Usage: wifi-connect <SSID> [password]%s\n", RED, NC);
            return 1;
        }
        const char *pass = (argc >= 4) ? argv[3] : "";
        return wifi_connect(argv[2], pass);
    }
    if (strcmp(argv[1], "wifi-scan") == 0) return wifi_scan();
    if (strcmp(argv[1], "wifi-disconnect") == 0) return wifi_disconnect();
    if (strcmp(argv[1], "wifi-check") == 0) return wifi_check_connection() ? 0 : 1;

    printf("  %s[ERROR] Unknown command: %s%s\n", RED, argv[1], NC);
    return 1;
}
#endif
