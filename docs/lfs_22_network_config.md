# Linux From Scratch: System Configuration: Network Setup

We configure network interfaces, hostnames, and loopback setups inside the LFS system.

---

## Detailed Explanation

Write configurations to `/etc/sysconfig/`:
```bash
# Hostname
echo "lfs-desktop" > /etc/hostname

# Network interface (e.g. eth0)
cat > /etc/sysconfig/ifconfig.eth0 << "EOF"
ONBOOT=yes
IFACE=eth0
SERVICE=ipv4-static
IP=192.168.1.100
GATEWAY=192.168.1.1
PREFIX=24
BROADCAST=192.168.1.255
EOF
```

---

## Best Practices & Tips

> [!TIP]
> Standard DHCP can be configured by installing `dhcpcd` or `udhcpc` binaries.
