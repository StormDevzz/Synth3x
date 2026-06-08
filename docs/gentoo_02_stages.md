# Gentoo Linux: Stage 1, 2, and 3 Explained

Gentoo installation starts from a compressed root archive called a Stage tarball.

---

## Detailed Explanation

Differences between stages:
- Stage 1: Contains only bootstrap compiler and libraries. The user compiles the entire toolchain.
- Stage 2: Toolchain is compiled; user compiles the system packages.
- Stage 3: Contains compiled toolchain and basic system files. This is the standard entry point.

---

## Best Practices & Tips

> [!TIP]
> Using Stage 3 accelerates installation while preserving configuration customization.
