# Gentoo Linux: Custom Kernels: Genkernel vs Manual Config

Gentoo provides two main methods to compile the Linux kernel: manual configuration or automated tools.

---

## Detailed Explanation

Manual configuration provides a minimal and optimized kernel:
```bash
cd /usr/src/linux
make menuconfig && make && make modules_install && make install
```
Genkernel automates hardware probe and initramfs compilation:
```bash
emerge sys-kernel/genkernel
genkernel all
```

---

## Best Practices & Tips

> [!TIP]
> Manual kernel configuration requires complete hardware knowledge but avoids initramfs overhead.
