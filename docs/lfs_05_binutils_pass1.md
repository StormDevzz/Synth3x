# Linux From Scratch: Compiling Binutils (Pass 1)

Binutils provides the linker (`ld`) and assembler (`as`). It is the first package built in the temporary toolchain phase.

---

## Detailed Explanation

We perform a cross-compiled build of Binutils:
```bash
tar -xf binutils-2.42.tar.xz
cd binutils-2.42
mkdir -v build
cd build
../configure --prefix=$LFS/tools \
             --with-sysroot=$LFS \
             --target=$LFS_TGT \
             --disable-nls \
             --enable-gprofng=no \
             --disable-werror \
             --enable-default-hash-style=gnu
make
make install
```

---

## Best Practices & Tips

> [!TIP]
> Pass 1 builds only the essential assembly and link utilities required for GCC Pass 1.
