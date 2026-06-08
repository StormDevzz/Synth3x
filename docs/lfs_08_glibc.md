# Linux From Scratch: Compiling Glibc (C Library)

Glibc (GNU C Library) provides the core system call wrappers and runtime environment for all Linux binaries.

---

## Detailed Explanation

We compile Glibc using the cross-toolchain compiled in previous steps:
```bash
tar -xf glibc-2.39.tar.xz
cd glibc-2.39
mkdir -v build && cd build
../configure --prefix=/usr \
             --host=$LFS_TGT \
             --build=$(../scripts/config.guess) \
             --enable-kernel=4.19 \
             --with-headers=$LFS/usr/include \
             libc_cv_slibdir=/usr/lib
make
make DESTDIR=$LFS install
```
Verify that basic dynamic linker commands work.

---

## Best Practices & Tips

> [!TIP]
> Make sure to test the compiled linker immediately to ensure compile viability.
