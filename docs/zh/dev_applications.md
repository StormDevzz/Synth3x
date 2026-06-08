# 开发者指南: 自定义应用程序

Synth3x 支持使用各种编程框架（C、C++、Rust、Go）和图形后端（GTK+3、SDL2、Cairo、egui）构建的自定义应用程序。您可以研究 `prog/` 目录中的现有模板。

---

## 1. 选择开发框架

*   **Rust (egui / eframe)：** 最适合跨平台、GPU 加速的图形界面（GUI）。（参见 `prog/AmnesiaIDE`）。
*   **C++ (SDL2 + Cairo)：** 最适合高性能渲染、自定义绘制画布和轻量级设置。（参见 `prog/FastWords` 和 `prog/SynPaint`）。
*   **C (GTK+3)：** 最适合传统的基于窗口的表单、菜单和文件对话框面板。（参见 `prog/Synth3x-FileMng` / `fileman`）。

---

## 2. 编译与构建集成

每个应用程序都必须有一个构建脚本（`Makefile` 或 `Cargo.toml`），用于将输出的二进制文件放置在可预测的位置。

### 将 App 添加到主构建流程中：
1.  将您的代码库放置在 `prog/your-app/` 下。
2.  在主 `Makefile` 中添加一个编译它的目标：
    ```makefile
    $(BUILD)/prog/your-app:
    	@mkdir -p $(BUILD)/prog
    	@make -C prog/your-app
    	@cp prog/your-app/binary $@
    ```
3.  将目标添加到 `PROGS` 变量列表中。

---

## 3. 部署与暂存

在系统安装期间，如果选中了暂存的 App，它们将被自动复制。请确保您创建了一个桌面启动器（`your-app.desktop`）：
```ini
[Desktop Entry]
Type=Application
Name=YourApp
Comment=My custom application
Exec=/usr/bin/your-app
Icon=utility
Terminal=false
Categories=Utility;
```
将此桌面启动器写入目标系统的 `/usr/share/applications/` 目录下。
