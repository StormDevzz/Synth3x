# Configuring make.conf for Optimizations

The file `/etc/portage/make.conf` controls build flags, parallel compilation settings, and package manager behaviors in Gentoo Linux.

---

## 1. Key Variables in `make.conf`

*   **COMMON_FLAGS:** Optimization flags passed to the compiler (`gcc` or `clang`).
    *   `-O2`: Recommended safe optimization level.
    *   `-pipe`: Use pipes instead of temporary files during compilation to speed up builds.
    *   `-march=native`: Target the CPU running the compilation.
*   **MAKEOPTS:** Control the number of parallel compiler threads. Generally set to the number of CPU threads:
    ```bash
    MAKEOPTS="-j$(nproc)"
    ```
*   **ACCEPT_KEYWORDS:** Select between stable (e.g. `amd64`) and testing (e.g. `~amd64`) packages.

---

## 2. Example Configuration

```bash
COMMON_FLAGS="-O2 -pipe -march=native"
CFLAGS="${COMMON_FLAGS}"
CXXFLAGS="${COMMON_FLAGS}"
MAKEOPTS="-j8"
USE="wayland dbus udev unicode -X"
```
