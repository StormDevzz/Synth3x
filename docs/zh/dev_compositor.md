# 开发者指南: Wayland 合成器 (AmnesiaDE)

Synth3x 中的自定义合成器位于 `src/compositor/` 目录下，并使用 DRM/KMS（直接渲染管理器/内核模式设置）编译为原生二进制文件，无需 X11 支持。

---

## 1. 代码库架构

合成器使用 C 语言和汇编语言编写：
*   `main.c`：初始化合成器状态、解析命令行参数并启动事件循环。
*   `drm.c`：处理帧缓冲区配置、硬件设备扫描、翻页（page flips）以及屏幕输出。
*   `input.c`：与 `libinput` 对接，捕获键盘、触摸板和鼠标输入。
*   `wl_server.c`：监听客户端连接并管理 Wayland 全局变量/注册表。
*   `shell.c`：控制应用程序布局、窗口堆叠、桌面边框和合成器状态。
*   `render.S`：底层汇编图形渲染流水线（基于软件的栅格化/位块传输）。

---

## 2. 修改快捷键键绑定

合成器快捷键键绑定在 `src/compositor/input.c` 中进行解析。要添加新的快捷键：
1.  找到键盘输入处理循环。
2.  添加对所需键码（使用来自 `<linux/input-event-codes.h>` 的 Linux 输入事件代码）的检查。
3.  实现您的动作。例如：
    ```c
    if (key == KEY_F1 && modifiers & MOD_LOGO) {
        spawn_terminal();
    }
    ```

---

## 3. 重新构建合成器

要在开发期间手动重新构建合成器：
```bash
make build/synth3x
```
要测试修改，可以使用 `ESC` 键退出桌面环境（DE）返回原始 tty1，然后运行构建的二进制文件：
```bash
./build/synth3x
```
