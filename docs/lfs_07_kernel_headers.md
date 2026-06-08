# Linux From Scratch: Linux Kernel API Headers Installation

The C library needs to know the system call interface exposed by the Linux kernel. This interface is defined by the kernel headers.

---

## Detailed Explanation

We extract the kernel headers and sanitize them for userspace usage:
```bash
tar -xf linux-6.7.4.tar.xz
cd linux-6.7.4
make mrproper
make headers
find usr/include -type f ! -name '*.h' -delete
mkdir -pv $LFS/usr/include
cp -rv usr/include/* $LFS/usr/include
```

---

## Best Practices & Tips

> [!TIP]
> Sanitized kernel headers represent the raw kernel interface and are platform-specific.
