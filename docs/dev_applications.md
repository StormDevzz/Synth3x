# Developer Guide: Custom Applications

Synth3x supports custom applications built with various programming frameworks (C, C++, Rust, Go) and graphical backends (GTK+3, SDL2, Cairo, egui). You can study existing templates in the `prog/` directory.

---

## 1. Choosing a Framework

*   **Rust (egui / eframe):** Best for cross-platform, GPU-accelerated GUIs. (See `prog/AmnesiaIDE`).
*   **C++ (SDL2 + Cairo):** Best for high-performance rendering, custom drawing canvases, and lightweight setups. (See `prog/FastWords` and `prog/SynPaint`).
*   **C (GTK+3):** Best for traditional window-based forms, menus, and file dialog panels. (See `prog/Synth3x-FileMng` / `fileman`).

---

## 2. Compilation and Build Integration

Each application must have a build script (`Makefile` or `Cargo.toml`) that places its output binary in a predictable location.

### Adding an App to the Main Build:
1.  Place your codebase under `prog/your-app/`.
2.  Add a target inside the main `Makefile` to compile it:
    ```makefile
    $(BUILD)/prog/your-app:
    	@mkdir -p $(BUILD)/prog
    	@make -C prog/your-app
    	@cp prog/your-app/binary $@
    ```
3.  Add the target to the `PROGS` variable list.

---

## 3. Staging and Deploying
Staged apps are copied automatically during system installations if selected. Ensure you create a desktop launcher (`your-app.desktop`):
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
Write this desktop entry to `/usr/share/applications/` on target systems.
