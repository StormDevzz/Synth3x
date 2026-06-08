# Gentoo Linux: Portage Overlays & Custom Ebuild Trees

Portage overlays allow users to add custom packages or override official ebuild definitions.

---

## Detailed Explanation

Configure overlays using `/etc/portage/repos.conf/`:
```
[my-overlay]
location = /var/db/repos/my-overlay
sync-type = git
sync-uri = https://github.com/user/overlay.git
auto-sync = yes
```
Enable overlays with `eselect repository`.

---

## Best Practices & Tips

> [!TIP]
> Use overlays to pin custom software like custom desktop environments or Wayland compositors.
