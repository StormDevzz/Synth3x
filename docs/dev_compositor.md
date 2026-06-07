# Developer Guide: Wayland Compositor (AmnesiaDE)

The custom compositor in Synth3x is located under `src/compositor/` and compiled as a native binary using DRM/KMS (Direct Rendering Manager / Kernel Mode Setting) without requiring X11.

---

## 1. Codebase Architecture

The compositor is written in C and Assembly:
*   `main.c`: Initializes the compositor state, parses command-line arguments, and starts the event loops.
*   `drm.c`: Handles framebuffer configurations, hardware device scanning, page flips, and screen outputs.
*   `input.c`: Interfaces with `libinput` to capture keyboard, touchpad, and mouse inputs.
*   `wl_server.c`: Listens to client connections and manages Wayland globals/registry.
*   `shell.c`: Controls application layout, window stacking, desktop borders, and compositor states.
*   `render.S`: Low-level Assembly graphics drawing pipeline (software-based rasterization/blitting).

---

## 2. Modifying Keybindings

Compositor keybindings are parsed in `src/compositor/input.c`. To add a new keybinding:
1.  Locate the keyboard input processing loop.
2.  Add a check for the desired keycode (using Linux input event codes from `<linux/input-event-codes.h>`).
3.  Implement your action. For example:
    ```c
    if (key == KEY_F1 && modifiers & MOD_LOGO) {
        spawn_terminal();
    }
    ```

---

## 3. Rebuilding the Compositor

To rebuild the compositor manually during development:
```bash
make build/synth3x
```
To test modifications, exit the DE using `ESC` to return to raw tty1, and run the built binary:
```bash
./build/synth3x
```
