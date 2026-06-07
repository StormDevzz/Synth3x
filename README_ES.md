# Synth3x

[Read in English](README.md)

Synth3x es un sistema operativo amnésico, endurecido y basado en fuentes que funciona con Gentoo Linux, un compositor Wayland personalizado escrito en C/Ensamblador (`AmnesiaDE`) y un instalador de procesos seguros en Rust.

---

## Inicio Rápido

### Compilar la ISO Live
```bash
./scripts/build_iso.sh
```

### Ejecutar en QEMU
```bash
make run-iso          # Iniciar el entorno Live
make run-installer    # Iniciar directamente en el instalador de terminal
```

---

## Leer la Documentación (Obligatorio)

Synth3x es un sistema operativo basado en código fuente que requiere comprender el funcionamiento interno. Antes de compilar, configurar o instalar el sistema, **debe** leer la documentación correspondiente:

*   **¿Cómo instalo el sistema operativo?**
    Siga los pasos detallados en la [Guía de Instalación de Gentoo](docs/es/gentoo_installation.md).
*   **¿Cómo funciona la seguridad endurecida de Gentoo?**
    Lea la [Guía de Seguridad de Gentoo Hardened](docs/es/gentoo_security.md).
*   **¿Cómo se configuran las USE flags y se administran los paquetes?**
    Consulte la [Guía del Gestor de Paquetes Portage](docs/es/gentoo_portage.md) para controlar la compilación de sus programas.
*   **¿Cómo optimizo las flags del sistema y acelero la compilación?**
    Configure `/etc/portage/make.conf` con la [Guía de Configuración de make.conf](docs/es/gentoo_makeconfig.md).
*   **¿Desea una descripción general de la arquitectura?**
    Lea la [Guía de Introducción y Filosofía de Gentoo](docs/es/gentoo_intro.md).

---

## Guías de Desarrollo (Obligatorio para Desarrolladores)

Si está desarrollando aplicaciones, paquetes o modificaciones para Synth3x, lea estas guías especializadas:

*   **¿Cómo desarrollo para el compositor Wayland?**
    Consulte la [Guía del Desarrollador del Compositor](docs/es/dev_compositor.md).
*   **¿Cómo escribo y empaqueto aplicaciones personalizadas?**
    Consulte la [Guía del Desarrollador de Aplicaciones](docs/es/dev_applications.md).
