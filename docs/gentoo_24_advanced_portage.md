# Gentoo Linux: Advanced Portage: Custom env files per package

Portage supports defining compile environments on a per-package basis.

---

## Detailed Explanation

Create custom environment settings in `/etc/portage/env/`:
```bash
# /etc/portage/env/no-ssp.conf
CFLAGS="${CFLAGS} -fno-stack-protector"
```
Map package to configuration in `/etc/portage/package.env`:
```
sys-libs/glibc no-ssp.conf
```

---

## Best Practices & Tips

> [!TIP]
> Use per-package environment settings to bypass optimization bugs without changing global setups.
