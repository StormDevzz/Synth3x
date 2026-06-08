# Linux From Scratch: Entering the Chroot Environment

We enter the virtual filesystem sandbox (chroot) to isolate target compilation from the host.

---

## Detailed Explanation

First, we mount virtual filesystems from the host:
```bash
mount -v --bind /dev $LFS/dev
mount -v --bind /dev/pts $LFS/dev/pts
mount -v -t proc proc $LFS/proc
mount -v -t sysfs sysfs $LFS/sys
mount -v -t tmpfs tmpfs $LFS/run
```

Then, enter the chroot jail:
```bash
chroot "$LFS" /usr/bin/env -i \
    HOME=/root \
    TERM="$TERM" \
    PS1='(lfs chroot) \w\\$ ' \
    PATH=/usr/bin:/usr/sbin \
    /bin/bash --login
```

---

## Best Practices & Tips

> [!TIP]
> Once in chroot, all references to root (`/`) map directly to the LFS target partition.
