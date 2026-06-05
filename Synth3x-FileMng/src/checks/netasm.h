#ifndef CHECKS_NETCHECK_H
#define CHECKS_NETCHECK_H

/* Assembly-level network checks */

/* TCP connect check: returns 1 if ip:port is reachable */
int net_check_connect(const char *ip, int port, int timeout_sec);

/* UDP DNS check: sends a minimal DNS query to 1.1.1.1:53 */
int net_check_dns(void);

/* UDP ping check: sends UDP packet to 1.1.1.1:80 */
int net_check_ping(void);

#endif
