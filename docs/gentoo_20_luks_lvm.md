# Gentoo Linux: Disk Encryption: LUKS + LVM Setup

Encrypting LFS or Gentoo storage partitions protects offline data integrity.

---

## Detailed Explanation

Creating a LUKS container and LVM groups:
```bash
cryptsetup luksFormat /dev/sdX2
cryptsetup open /dev/sdX2 cryptroot

# Create LVM volumes
pvcreate /dev/mapper/cryptroot
vgcreate vg0 /dev/mapper/cryptroot
lvcreate -L 20G -n root vg0
```

---

## Best Practices & Tips

> [!TIP]
> Configure custom initramfs scripts to prompt for encryption passwords during boot.
