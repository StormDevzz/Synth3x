# Gentoo Linux: Package Masking & Unmasking (accept_keywords)

Gentoo uses keywords to manage testing package compilation.

---

## Detailed Explanation

Keywords are:
- `amd64`: Stable package.
- `~amd64`: Testing package.
- `-*`: Masked / incompatible package.

Unmasking a package in `/etc/portage/package.accept_keywords/`:
```
app-editors/vscode ~amd64
```

---

## Best Practices & Tips

> [!TIP]
> Keep testing keywords localized to avoid breaking world dependency stability.
