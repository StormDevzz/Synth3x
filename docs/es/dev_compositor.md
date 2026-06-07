# Guía del Desarrollador: Compositor Wayland (AmnesiaDE)

El compositor personalizado de Synth3x se encuentra bajo la ruta `src/compositor/` y se compila como un binario nativo que se comunica directamente con DRM/KMS sin necesidad de X11.

---

## 1. Arquitectura de Código

El compositor se estructura en C y código ensamblador:
*   `main.c`: Inicializa el bucle de eventos principal.
*   `drm.c`: Gestiona el búfer de fotogramas (framebuffer) y la salida a pantalla.
*   `input.c`: Captura eventos de teclado, ratón y panel táctil mediante `libinput`.
*   `render.S`: Rutinas de dibujo de bajo nivel optimizadas en código ensamblador.

---

## 2. Modificación de Atajos de Teclado

Los atajos de teclado se definen en `src/compositor/input.c`.
Para agregar una combinación:
1.  Busque el bucle que procesa los códigos de teclas.
2.  Agregue una condición usando los códigos estándar de Linux (p. ej., `KEY_F1`).
    ```c
    if (key == KEY_F1 && modifiers & MOD_LOGO) {
        spawn_terminal();
    }
    ```
