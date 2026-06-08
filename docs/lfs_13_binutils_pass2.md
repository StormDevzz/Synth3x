# Linux From Scratch: Compiling Binutils (Pass 2)

Binutils Pass 2 utilizes the target compiler libraries compiled in chroot to produce target-native linkers.

---

## Detailed Explanation

Configure and build within the chroot environment:
```bash
tar -xf binutils-2.42.tar.xz
cd binutils-2.42
mkdir -v build && cd build
../configure --prefix=/usr \
             --enable-gold \
             --enable-ld=default \
             --enable-plugins \
             --enable-shared \
             --disable-werror \
             --enable-64-bit-bfd \
             --with-system-zlib
make tooldir=/usr
make tooldir=/usr install
```

---

## Best Practices & Tips

> [!TIP]
> Pass 2 links with target libraries directly, making the resulting binary native.
