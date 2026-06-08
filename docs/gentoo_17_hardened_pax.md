# Gentoo Linux: Gentoo Hardened: PaX, grsecurity, and SSP

Gentoo Hardened is a project focusing on building secure and isolated environments.

---

## Detailed Explanation

Core components include:
- PaX: Kernel-level protection preventing writable memory pages from being executed.
- grsecurity: Role-based access control and hardening patches.
- SSP (Stack Smashing Protection): Compiler canaries checking for buffer limits.

---

## Best Practices & Tips

> [!TIP]
> Combining compiler flags and kernel isolation provides advanced privilege protection.
