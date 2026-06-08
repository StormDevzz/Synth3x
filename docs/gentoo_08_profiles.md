# Gentoo Linux: Profiles & Hardened Configurations

Profiles define default configuration profiles, USE flags, and package versions for Portage.

---

## Detailed Explanation

Listing available profiles:
```bash
eselect profile list
```
Selecting a hardened profile:
```bash
eselect profile set default/linux/amd64/17.1/hardened
```

---

## Best Practices & Tips

> [!TIP]
> The hardened profile switches the toolchain to compile secure binaries automatically.
