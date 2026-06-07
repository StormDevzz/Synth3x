# Gentoo Linux: Introducción y Filosofía

Gentoo Linux es una distribución de Linux única basada en código fuente, diseñada en torno a la filosofía de personalización total, rendimiento y elección del usuario. A diferencia de las distribuciones precompiladas más comunes (como Ubuntu, Fedora o Debian), Gentoo le brinda las herramientas para compilar y construir su sistema operativo directamente en su máquina.

---

## 1. La Filosofía Principal

La filosofía central de Gentoo es la **libre elección**. Cada aspecto del sistema operativo puede ser modificado a gusto:
*   **Opciones de compilación:** Elija exactamente qué características incluir en sus programas.
*   **Sistema de inicio:** Seleccione entre OpenRC (ligero, predeterminado) o systemd.
*   **Perfiles de sistema:** Seleccione perfiles de escritorio, servidor, endurecido (hardened) o desarrollador.

---

## 2. ¿Por qué utilizar un SO basado en fuentes?

1.  **Rendimiento a Medida:** Los paquetes se compilan con flags específicas para su arquitectura de CPU (p. ej., `-march=native`), desbloqueando optimizaciones de instrucciones.
2.  **Sin software innecesario (Bloat):** Al deshabilitar las funciones que no necesita, reduce el tamaño de los binarios, el consumo de memoria y la superficie de ataque de seguridad.
