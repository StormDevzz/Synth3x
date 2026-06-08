# Linux From Scratch: Libstdc++ from GCC (Pass 1)

Libstdc++ is the standard C++ library. It is built after Glibc is available so that it compiles with target headers.

---

## Detailed Explanation

We compile libstdc++ separately:
```bash
cd gcc-13.2.0/build
../libstdc++-v3/configure --host=$LFS_TGT \
                          --build=$(../config.guess) \
                          --prefix=/usr \
                          --disable-multilib \
                          --disable-nls \
                          --disable-libstdcxx-pch \
                          --with-gxx-include-dir=/tools/$LFS_TGT/include/c++/13.2.0
make
make DESTDIR=$LFS install
```

---

## Best Practices & Tips

> [!TIP]
> This library supports basic C++ utilities in the bootstrap phase.
