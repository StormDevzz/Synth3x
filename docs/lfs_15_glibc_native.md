# Linux From Scratch: Compiling Glibc & Verification

Glibc is recompiled natively inside the chroot environment to ensure all library hooks and locale databases are built.

---

## Detailed Explanation

Compiling native C library and installing locales:
```bash
tar -xf glibc-2.39.tar.xz
cd glibc-2.39
mkdir -v build && cd build
../configure --prefix=/usr \
             --disable-profile \
             --enable-kernel=4.19 \
             --enable-stack-protector=strong \
             libc_cv_slibdir=/usr/lib
make
make install

# Generate locales
localedef -i en_US -f UTF-8 en_US.UTF-8
```

---

## Best Practices & Tips

> [!TIP]
> Verification: compiled dynamic binaries must refer only to `/lib/ld-linux-x86-64.so.2`.
