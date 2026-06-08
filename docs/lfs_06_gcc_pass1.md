# Linux From Scratch: Compiling GCC (Pass 1)

GCC (GNU Compiler Collection) is the core compiler. Pass 1 constructs a bootstrap compiler utilizing Binutils Pass 1.

---

## Detailed Explanation

GCC requires GMP, MPFR, and MPC libraries. Place them inside the source tree:
```bash
tar -xf gcc-13.2.0.tar.xz
cd gcc-13.2.0
tar -xf ../gmp-6.3.0.tar.xz && mv -v gmp-6.3.0 gmp
tar -xf ../mpfr-4.2.1.tar.xz && mv -v mpfr-4.2.1 mpfr
tar -xf ../mpc-1.3.1.tar.gz && mv -v mpc-1.3.1 mpc

mkdir -v build && cd build
../configure --target=$LFS_TGT \
             --prefix=$LFS/tools \
             --with-glibc-version=2.39 \
             --with-sysroot=$LFS \
             --enable-default-pie \
             --enable-default-ssp \
             --disable-nls \
             --disable-shared \
             --disable-multilib \
             --disable-threads \
             --disable-libatomic \
             --disable-libgomp \
             --disable-libquadmath \
             --disable-libssp \
             --disable-libvtv \
             --disable-libstdcxx \
             --enable-languages=c,c++
make
make install
```

---

## Best Practices & Tips

> [!TIP]
> This bootstrap compiler will compile the C library (Glibc) for the LFS target system.
