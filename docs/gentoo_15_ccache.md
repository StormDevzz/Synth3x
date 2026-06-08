# Gentoo Linux: Ccache: Compilation Caching

Ccache caches compilation results, speeding up re-compilation of packages.

---

## Detailed Explanation

Enable Ccache globally in `/etc/portage/make.conf`:
```
FEATURES="ccache"
CCACHE_SIZE="10G"
```
Configure cache directory ownership to portage.

---

## Best Practices & Tips

> [!TIP]
> CCache is highly effective when updating package revisions frequently.
