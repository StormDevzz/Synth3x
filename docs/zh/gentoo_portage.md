# Portage: Gentoo 软件包管理器

Portage 是 Gentoo Linux 的官方软件包管理和编译系统。它用于追踪依赖关系、编译源码包并配置环境参数。

---

## 1. 常用 Portage 命令

*   **安装软件包：**
    ```bash
    emerge --ask category/package-name
    ```
*   **卸载软件包：**
    ```bash
    emerge --depclean package-name
    ```
*   **搜索软件包：**
    ```bash
    emerge --search query
    ```
*   **同步系统源：**
    ```bash
    emerge --sync
    ```

---

## 2. USE 标志

USE 标志是定义软件包可选编译时功能的关键字。
*   **全局 USE 标志：** 在 `/etc/portage/make.conf` 中设置（适用于所有软件包）。
    ```bash
    USE="wayland dbus unicode -X -kde"
    ```
*   **单包 USE 标志：** 在 `/etc/portage/package.use/custom` 中配置。
    ```bash
    www-client/firefox wayland dbus
    ```
