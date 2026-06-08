# Linux From Scratch: Compiling Sed, Tar, and Xz

Text replacement (`sed`) and archiving tools (`tar`, `xz`) are required to bootstrap any package manager.

---

## Detailed Explanation

Compiling compression toolchain:
```bash
# Tar
FORCE_UNSAFE_CONFIGURE=1 ./configure --prefix=/usr
make && make install

# Sed
./configure --prefix=/usr
make && make install
```

---

## Best Practices & Tips

> [!TIP]
> Ensure all compression library formats (xz, gzip, bzip2) are supported by Tar.
