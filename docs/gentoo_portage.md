# Portage: The Gentoo Package Manager

Portage is the official package management and compilation system for Gentoo Linux. It tracks dependencies, compiles source packages, and configures environment parameters.

---

## 1. Essential Portage Commands

*   **Installing Packages:**
    ```bash
    emerge --ask category/package-name
    ```
*   **Uninstalling Packages:**
    ```bash
    emerge --depclean package-name
    ```
*   **Searching Packages:**
    ```bash
    emerge --search query
    ```
*   **System Synchronization:**
    ```bash
    emerge --sync
    ```

---

## 2. USE Flags

USE flags are keywords that define optional compile-time features for packages.
*   **Global USE Flags:** Set in `/etc/portage/make.conf` (applies to all packages).
    ```bash
    USE="wayland dbus unicode -X -kde"
    ```
*   **Per-Package USE Flags:** Configured in `/etc/portage/package.use/custom`.
    ```bash
    www-client/firefox wayland dbus
    ```
