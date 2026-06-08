# Gentoo Linux: make.conf: GCC Optimization Flags

The `/etc/portage/make.conf` file controls compile optimizations, mirrors, and environment configurations.

---

## Detailed Explanation

Example optimized config:
```
COMMON_FLAGS="-O2 -pipe -march=native"
CFLAGS="${COMMON_FLAGS}"
CXXFLAGS="${COMMON_FLAGS}"
MAKEOPTS="-j8"
PORTAGE_NICENESS=19
```

---

## Best Practices & Tips

> [!TIP]
> Avoid using `-O3` globally; it can trigger compile instability and larger binary sizes.
