# Gentoo Linux: USE Flags: Deep Customization & Profiles

USE flags are Portage compile options that determine which features are built into software.

---

## Detailed Explanation

Define USE flags globally in `/etc/portage/make.conf`:
```
USE="wayland elogind dbus -X -gnome -kde"
```
Define package-specific flags in `/etc/portage/package.use/`:
```
media-video/ffmpeg vpx x264 opus
```

---

## Best Practices & Tips

> [!TIP]
> Removing unused USE flags (like `-X` or `-gtk`) reduces compile time and dependency footprints.
