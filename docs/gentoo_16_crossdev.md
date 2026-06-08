# Gentoo Linux: Crossdev: Cross-Compilation Toolchains

Crossdev is a Gentoo tool to construct cross-compilation toolchains easily.

---

## Detailed Explanation

Installing Crossdev and building a target toolchain (e.g. ARM64):
```bash
emerge sys-devel/crossdev
crossdev -t aarch64-unknown-linux-gnu
```
This creates target compiler libraries under portage namespaces.

---

## Best Practices & Tips

> [!TIP]
> Use crossdev to build custom packages for embedded targets.
