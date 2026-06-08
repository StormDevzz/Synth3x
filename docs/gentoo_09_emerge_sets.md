# Gentoo Linux: Emerge: Dependencies, Slots, and Sets

Emerge handles dependency calculations and dependency slots.

---

## Detailed Explanation

Gentoo supports Slotting, allowing multiple versions of a package (e.g. Python 3.10 and 3.11) to coexist.

Using package sets:
- `@world`: All user-installed packages.
- `@system`: Core operating system packages.
- `@preserved-rebuild`: Packages requiring rebuild after library updates.

---

## Best Practices & Tips

> [!TIP]
> Run `emerge --depclean` regularly to clean orphaned dependency libraries.
