# Linux From Scratch: Kernel Configuration: Drivers and File Systems

Configuring the Linux kernel requires selecting the correct device drivers, filesystem formats, and CPU architectures.

---

## Detailed Explanation

Configure using menuconfig:
```bash
cd /usr/src/linux-6.7.4
make menuconfig
```
Ensure the following are compiled built-in (not as modules) if you are booting without initramfs:
- Boot drive interface driver (SATA/NVMe)
- Root file system driver (ext4/xfs)
- VT console & Framebuffer console (`CONFIG_VT`, `CONFIG_FRAMEBUFFER_CONSOLE`)

---

## Best Practices & Tips

> [!TIP]
> Verify CPU flags to compile optimized instruction sets.
