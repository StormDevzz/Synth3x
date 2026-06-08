# Gentoo Linux: Gentoo Hardened: Compiler Hardening (SSP & PIE)

Compiler hardening options protect compiled binaries from memory execution hijacking.

---

## Detailed Explanation

Flags configured by the hardened profile:
- `-fstack-protector-strong`: Injects guards to check stack frame boundaries.
- `-fPIE -pie`: Compiles binaries to support memory randomized locations (ASLR).

---

## Best Practices & Tips

> [!TIP]
> Hardening flags decrease execution performance slightly but prevent buffer exploits.
