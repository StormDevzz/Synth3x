#include "netasm.h"
#include "../fm.h"

int net_internet_check(void) {
    /* Try TCP connect to Cloudflare (1.1.1.1:80) */
    if (net_check_connect("1.1.1.1", 80, 3)) return 1;

    /* Try TCP connect to Google DNS (8.8.8.8:53) */
    if (net_check_connect("8.8.8.8", 53, 3)) return 1;

    /* Try DNS query */
    if (net_check_dns()) return 1;

    /* Try UDP ping */
    if (net_check_ping()) return 1;

    return 0;
}

const char *net_status(void) {
    int dns = net_check_dns();
    int tcp = net_check_connect("1.1.1.1", 80, 2);
    int ping = net_check_ping();

    if (tcp && dns) return "internet: connected";
    if (dns) return "internet: DNS OK, HTTP blocked";
    if (ping) return "internet: limited";
    return "internet: offline";
}

int net_test_all(void) {
    update_status("testing internet connectivity...");

    int tcp = net_check_connect("1.1.1.1", 80, 3);
    update_status("TCP 1.1.1.1:80 — %s", tcp ? "OK" : "FAIL");

    int dns = net_check_dns();
    update_status("DNS 1.1.1.1:53 — %s", dns ? "OK" : "FAIL");

    int ping = net_check_ping();
    update_status("UDP 1.1.1.1:80 — %s", ping ? "OK" : "FAIL");

    int gcp = net_check_connect("8.8.8.8", 53, 3);
    update_status("TCP 8.8.8.8:53 — %s", gcp ? "OK" : "FAIL");

    if (tcp || dns || ping || gcp) {
        update_status("internet: connected");
        return 1;
    }
    update_status("internet: offline");
    return 0;
}
