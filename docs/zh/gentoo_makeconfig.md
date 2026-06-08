# 为系统优化配置 make.conf

`/etc/portage/make.conf` 文件控制着 Gentoo Linux 中的构建标志、并行编译设置以及软件包管理器行为。

---

## 1. `make.conf` 中的关键变量

*   **COMMON_FLAGS：** 传递给编译器（`gcc` 或 `clang`）的优化标志。
    *   `-O2`：推荐的安稳优化级别。
    *   `-pipe`：在编译期间使用管道而不是临时文件，以此加快构建速度。
    *   `-march=native`：生成针对运行编译的 CPU 进行优化的代码。
*   **MAKEOPTS：** 控制并行编译器线程的数量。通常设置为 CPU 线程数：
    ```bash
    MAKEOPTS="-j$(nproc)"
    ```
*   **ACCEPT_KEYWORDS：** 在稳定包（例如 `amd64`）和测试包（例如 `~amd64`）之间进行选择。

---

## 2. 示例配置

```bash
COMMON_FLAGS="-O2 -pipe -march=native"
CFLAGS="${COMMON_FLAGS}"
CXXFLAGS="${COMMON_FLAGS}"
MAKEOPTS="-j8"
USE="wayland dbus udev unicode -X"
```
