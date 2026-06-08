# Linux From Scratch: Bootloader: Installing and Configuring GRUB

GRUB (Grand Unified Bootloader) loads the Linux kernel and passes command line options.

---

## Detailed Explanation

Installing GRUB to the boot disk:
```bash
grub-install --target=x86_64-efi --efi-directory=/boot --bootloader-id=LFS --recheck
```

Writing `/boot/grub/grub.cfg`:
```
set timeout=5
set default=0

menuentry "Linux From Scratch, Kernel 6.7.4" {
    linux /vmlinuz-6.7.4-lfs root=/dev/sda2 ro loglevel=3
}
```

---

## Best Practices & Tips

> [!TIP]
> Ensure the root kernel argument points to the correct partition UUID or device identifier.
