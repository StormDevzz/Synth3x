# Gentoo Linux: Gentoo Networking: Netifrc vs NetworkManager

Gentoo provides different options to manage network interfaces and routing tables.

---

## Detailed Explanation

Netifrc is Gentoo's modular network system configured via `/etc/conf.d/net`:
```
config_eth0="dhcp"
```
NetworkManager handles dynamic WiFi and cellular connections via dbus APIs.

---

## Best Practices & Tips

> [!TIP]
> Use netifrc for server and custom static environments, and NetworkManager for desktops.
