# Linux From Scratch: Partitioning & Filesystem Prep

Building LFS requires a dedicated storage location, typically a clean hard disk partition. We create a partition topology that conforms to modern UEFI boot systems.

---

## Detailed Explanation

Example partitioning using `fdisk` or `parted`:
1. EFI System Partition (ESP): 512MB formatted as FAT32.
2. Root Partition: Remaining disk formatted as ext4.

Formatting commands:
```bash
mkfs.vfat -F32 /dev/sdX1
mkfs.ext4 /dev/sdX2
```

Setting up the LFS mount point variable:
```bash
export LFS=/mnt/lfs
mkdir -pv $LFS
mount -v -t ext4 /dev/sdX2 $LFS
```

---

## Best Practices & Tips

> [!TIP]
> Always define the `$LFS` environment variable in your shell profile to prevent accidental commands on the host system.
