# Gentoo Linux: Init Systems: OpenRC Internals & Scripting

OpenRC is the default service manager on Gentoo, relying on dependency-based init script parsing.

---

## Detailed Explanation

Managing services using rc-update:
```bash
# Start service on boot
rc-update add tor default

# Start service manually
rc-service tor start
```
OpenRC scripts are written in shell script with specific `depend` and `start` functions.

---

## Best Practices & Tips

> [!TIP]
> OpenRC supports running services in parallel to decrease system boot times.
