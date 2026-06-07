# Service Management with OpenRC in Gentoo

OpenRC is Gentoo's default, dependency-based init system. It manages boot services, system initialization, and daemon processes.

---

## 1. Controlling Services

OpenRC uses the `rc-service` command to start, stop, restart, and inspect system services.

*   **Start a Service:**
    ```bash
    rc-service tor start
    ```
*   **Stop a Service:**
    ```bash
    rc-service tor stop
    ```
*   **Restart a Service:**
    ```bash
    rc-service tor restart
    ```
*   **Inspect Service Status:**
    ```bash
    rc-service tor status
    ```

---

## 2. Managing Runlevels

Runlevels control which services start during system bootup. OpenRC uses `rc-update` to manage services in runlevels.

*   **Add a Service to Boot Runlevel:**
    ```bash
    rc-update add dbus boot
    ```
*   **Add a Service to Default Runlevel:**
    ```bash
    rc-update add tor default
    ```
*   **Remove a Service from Default Runlevel:**
    ```bash
    rc-update delete tor default
    ```
*   **Show All Registered Services:**
    ```bash
    rc-status --all
    ```
