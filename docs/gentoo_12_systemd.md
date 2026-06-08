# Gentoo Linux: Init Systems: Systemd on Gentoo

Gentoo can be compiled to use systemd as the init system instead of OpenRC.

---

## Detailed Explanation

Requires selecting a systemd-enabled profile and compile flag:
```
USE="systemd -openrc"
```
Rebuilding the system to apply systemd configurations:
```bash
emerge --ask --newuse --deep @world
```

---

## Best Practices & Tips

> [!TIP]
> Switching profiles requires checking configuration files beforehand.
