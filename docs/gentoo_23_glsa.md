# Gentoo Linux: Gentoo Security Auditing: GLSA and Security Updates

GLSA (Gentoo Linux Security Advisory) tracks known vulnerabilities in packages.

---

## Detailed Explanation

Audit packages using GLSA tools:
```bash
emerge app-portage/gentoolkit
glsa-check --list
glsa-check --test affected
```
Recompile affected packages automatically.

---

## Best Practices & Tips

> [!TIP]
> Run GLSA audits regularly as part of system update routines.
