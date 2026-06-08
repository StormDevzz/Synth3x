# Linux From Scratch: System Configuration: fstab & Mounts

The `/etc/fstab` file defines mounting points and virtual filesystems loaded during initialization.

---

## Detailed Explanation

Example `/etc/fstab` definition:
```
# <file system> <mount point>   <type>  <options>       <dump>  <pass>
/dev/sda2       /               ext4    noatime,defaults 1       1
/dev/sda1       /boot           vfat    defaults         0       2
proc            /proc           proc    nosuid,noexec    0       0
sysfs           /sys            sysfs   nosuid,noexec    0       0
devpts          /dev/pts        devpts  gid=5,mode=620   0       0
tmpfs           /run            tmpfs   defaults         0       0
devtmpfs        /dev            devtmpfs mode=0755,nosuid 0      0
```

---

## Best Practices & Tips

> [!TIP]
> Incorrect mount UUIDs or descriptors will halt the boot sequence during system initialization.
