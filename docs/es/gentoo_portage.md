# Portage: El Gestor de Paquetes de Gentoo

Portage es el sistema oficial de administración y compilación de paquetes para Gentoo Linux. Se encarga de rastrear dependencias, compilar el código fuente y configurar variables de entorno.

---

## 1. Comandos Esenciales de Portage

*   **Instalar Paquetes:**
    ```bash
    emerge --ask categoria/nombre-paquete
    ```
*   **Desinstalar Paquetes:**
    ```bash
    emerge --depclean nombre-paquete
    ```
*   **Actualizar el Sistema:**
    ```bash
    emerge -uDNav @world
    ```

---

## 2. USE Flags

Las flags USE definen qué características opcionales compilar en los programas.
*   **Configuración Global:** Se define en `/etc/portage/make.conf`.
    ```bash
    USE="wayland dbus unicode -X -kde"
    ```
*   **Por Paquete:** Se configura en `/etc/portage/package.use/custom`.
    ```bash
    www-client/firefox wayland dbus
    ```
