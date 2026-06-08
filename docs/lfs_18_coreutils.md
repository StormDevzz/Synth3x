# Linux From Scratch: Compiling Bash, Coreutils, and Diffutils

Core utilities like `ls`, `cp`, `mv`, and shell interfaces like `bash` form the user space environment.

---

## Detailed Explanation

Bootstrapping shell utilities inside chroot:
```bash
# Compile Bash
./configure --prefix=/usr --with-installed-readline
make && make install

# Compile Coreutils
FORCE_UNSAFE_CONFIGURE=1 ./configure --prefix=/usr --enable-no-install-program=kill,uptime
make && make install
```

---

## Best Practices & Tips

> [!TIP]
> Testing the coreutils ensures that operations like file copying and moving work correctly.
