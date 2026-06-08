# Synth3x

[Read in English](README.md)

Synth3x 是一个无痕（Amnesic）、加固、基于源码构建的操作系统。它基于 Gentoo Linux 开发，使用自定义的 C/汇编 Wayland 合成器（`AmnesiaDE`）以及基于 Rust 开发的安全进程安装器。

---

## 快速入门

### 构建 Live ISO
```bash
./scripts/build_iso.sh
```

### 在 QEMU 中运行
```bash
make run-iso          # 启动 Live 环境
make run-installer    # 直接启动到终端安装程序
```

---

## 阅读文档（必读）

Synth3x 是一个基于源码构建的操作系统，要求您了解其内部工作原理。在构建、配置或安装系统之前，您**必须**阅读相关文档：

*   **如何安装操作系统？**
    请按照 [Gentoo 安装指南](docs/zh/gentoo_installation.md) 中的步骤进行操作。
*   **Gentoo 加固安全（Hardened）是如何工作的？**
    阅读 [Gentoo 加固安全指南](docs/zh/gentoo_security.md)。
*   **如何配置 USE 标志并管理软件包？**
    阅读 [Portage 包管理器指南](docs/zh/gentoo_portage.md) 以控制软件包的编译方式。
*   **如何优化系统标志并加快编译速度？**
    参考 [make.conf 配置指南](docs/zh/gentoo_makeconfig.md) 来配置 `/etc/portage/make.conf`。
*   **想要了解系统架构的概述？**
    阅读 [Gentoo 简介与哲学指南](docs/zh/gentoo_intro.md)。

---

## 开发者指南（开发者必读）

如果您正在为 Synth3x 开发应用程序、软件包或进行修改，请阅读以下专业指南：

*   **如何为 Wayland 合成器进行开发？**
    阅读 [合成器开发指南](docs/zh/dev_compositor.md)。
*   **如何编写和打包自定义应用程序？**
    阅读 [应用程序开发指南](docs/zh/dev_applications.md)。
