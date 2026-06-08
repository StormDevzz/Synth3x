# Linux From Scratch: Compiling BusyBox (Minimalist Init)

BusyBox combines tiny versions of many common UNIX utilities into a single executable, useful for custom initramfs.

---

## Detailed Explanation

Busybox config and compile commands:
```bash
tar -xf busybox-1.36.1.tar.bz2
cd busybox-1.36.1
make defconfig
# Customize config to use static linking if desired
make
make install CONFIG_PREFIX=/usr/local/busybox
```

---

## Best Practices & Tips

> [!TIP]
> BusyBox is frequently used to provide basic shell and file commands inside an initramfs environment.
