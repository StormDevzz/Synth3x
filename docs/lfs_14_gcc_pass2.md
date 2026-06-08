# Linux From Scratch: Compiling GCC (Pass 2)

GCC Pass 2 builds the compiler natively within the chroot environment, outputting compilers bound to the LFS Glibc.

---

## Detailed Explanation

We compile native GCC and verify the toolchain sanity:
```bash
tar -xf gcc-13.2.0.tar.xz
cd gcc-13.2.0
# GMP, MPFR, MPC are now compiled natively in the system
mkdir -v build && cd build
../configure --prefix=/usr \
             LD=/usr/bin/ld \
             --enable-languages=c,c++ \
             --enable-default-pie \
             --enable-default-ssp \
             --disable-multilib \
             --disable-bootstrap \
             --with-system-zlib
make
make install
```

---

## Best Practices & Tips

> [!TIP]
> Perform a compilation sanity test after GCC Pass 2 using a dummy C file.
