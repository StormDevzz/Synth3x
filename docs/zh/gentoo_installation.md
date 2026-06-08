# Gentoo Linux: 分步安装概述

安装 Gentoo Linux 是一个手动过程，涉及配置存储、解压基础文件、chroot 进入目标环境以及构建内核/引导加载程序。

---

## 安装流程

1.  **启动 Live 环境：** 启动最小安装 CD 或像 Synth3x 这样的 Live 环境。
2.  **配置网络：** 建立网络连接以便下载软件包和文件。
3.  **准备存储空间：**
    *   创建 GPT 或 MBR 分区表。
    *   创建分区：EFI 分区（引导）和 Root 根分区（ext4/btrfs）。
    *   格式化分区并将其挂载到 `/mnt/gentoo` 下。
4.  **解压 Stage3：** 下载并解压与您的系统配置文件相对应的 Stage3 归档文件。
5.  **Chroot 进入新系统：**
    *   挂载宿主机的虚拟文件系统（`/proc`、`/sys`、`/dev`）。
    *   进入目标环境：`chroot /mnt/gentoo /bin/bash`。
6.  **配置 Portage：** 同步源存储库并在 `/etc/portage/make.conf` 中配置编译标志。
7.  **构建内核：** 配置并编译 Linux 内核。
8.  **设置系统配置：** 定义 `/etc/fstab`、主机名、本地化语言（locales）和网络配置。
9.  **安装引导程序：** 部署 GRUB 并重启进入您全新的 Gentoo 系统！
