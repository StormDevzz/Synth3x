# Gentoo Linux: Ebuilds: How Ebuild Scripts Work

An ebuild is a bash script describing the download, compilation, and installation steps of a package.

---

## Detailed Explanation

Key ebuild phases:
- `src_unpack()`: Unpacks source tarballs.
- `src_prepare()`: Applies patches.
- `src_configure()`: Passes build flags.
- `src_compile()`: Runs make / build utilities.
- `src_install()`: Copies binaries to stage directory.

---

## Best Practices & Tips

> [!TIP]
> Writing clean ebuilds requires following Gentoo's QA guidelines.
