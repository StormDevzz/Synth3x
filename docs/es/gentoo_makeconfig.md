# Configuración de make.conf para Optimizaciones

El archivo `/etc/portage/make.conf` controla las variables de compilación, paralelismo y el comportamiento general del gestor de paquetes Portage.

---

## 1. Variables Clave en `make.conf`

*   **COMMON_FLAGS:** Parámetros de optimización pasados al compilador (`gcc` o `clang`).
    *   `-O2`: Nivel de optimización recomendado y seguro.
    *   `-pipe`: Usa tuberías de memoria en vez de archivos temporales en disco para agilizar compilaciones.
    *   `-march=native`: Optimiza el binario específicamente para el procesador de la máquina actual.
*   **MAKEOPTS:** Controla el número de hilos del compilador. Se recomienda definirlo según los hilos del procesador:
    ```bash
    MAKEOPTS="-j$(nproc)"
    ```

---

## 2. Ejemplo de Configuración

```bash
COMMON_FLAGS="-O2 -pipe -march=native"
CFLAGS="${COMMON_FLAGS}"
CXXFLAGS="${COMMON_FLAGS}"
MAKEOPTS="-j4"
USE="wayland dbus udev unicode -X"
```
