# Linux From Scratch: Creating Directories and Files (FHS)

An operating system requires a standard directory layout conformant to the Filesystem Hierarchy Standard (FHS).

---

## Detailed Explanation

Creating FHS directories inside the chroot environment:
```bash
mkdir -pv /{boot,home,mnt,opt,srv}
mkdir -pv /etc/{opt,sysconfig}
mkdir -pv /lib/firmware
mkdir -pv /media/{floppy,cdrom}
mkdir -pv /usr/{,local/}{bin,include,lib,sbin,src}
mkdir -pv /usr/{,local/}share/{color,dict,doc,info,locale,man}
mkdir -pv /usr/{,local/}share/man/man{1..8}
mkdir -pv /var/{cache,local,lock,log,mail,opt,run,spool}
mkdir -pv /var/lib/{color,misc,locate}
```

---

## Best Practices & Tips

> [!TIP]
> Creating correct ownerships and file permissions on system directories is crucial for multi-user security.
