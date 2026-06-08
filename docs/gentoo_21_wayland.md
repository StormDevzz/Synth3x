# Gentoo Linux: Desktop Environments: Wayland & Xorg Config

Configuring graphics on Gentoo requires compile flag definitions and driver setups.

---

## Detailed Explanation

USE flag setting for Wayland environments:
```
USE="wayland elogind -X"
```
Configure graphic card identifiers in `make.conf`:
```
VIDEO_CARDS="intel amdgpu nouveau"
```

---

## Best Practices & Tips

> [!TIP]
> Wayland provides process isolation, avoiding X11 protocol vulnerabilities.
