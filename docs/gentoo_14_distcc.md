# Gentoo Linux: Distcc: Distributed Compiling

Distcc distributes C/C++ compilation jobs across a network of compile machines.

---

## Detailed Explanation

Configure `/etc/portage/make.conf` to enable distcc:
```
FEATURES="distcc"
MAKEOPTS="-j16"
```
Set compiler host IPs in `/etc/distcc/hosts`.

---

## Best Practices & Tips

> [!TIP]
> Distcc speeds up compilation on slow target devices like laptops.
