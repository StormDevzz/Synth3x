# Gentoo Kernel Configuration & Compilation

The Linux kernel is the core of the operating system. In Gentoo Linux, users can choose between pre-configured kernels or building custom kernels.

---

## 1. Kernel Installation Choices

1.  **Distribution Kernels (Binary/Source):**
    Pre-configured kernels maintained by Gentoo package maintainers. Simple to install and upgrade.
    ```bash
    emerge sys-kernel/gentoo-kernel-bin
    ```
2.  **Manual Custom Kernel (Recommended for Experts):**
    Provides absolute control over drivers, subsystems, and performance optimizations.
    ```bash
    emerge sys-kernel/gentoo-sources
    ```

---

## 2. Compiling Manually

To configure and build a manual kernel:
1.  Navigate to kernel directory:
    ```bash
    cd /usr/src/linux
    ```
2.  Open configuration menu:
    ```bash
    make menuconfig
    ```
3.  Select built-in drivers for graphics card, filesystems, and network adapters.
4.  Build and install:
    ```bash
    make -j$(nproc) && make modules_install && make install
    ```
