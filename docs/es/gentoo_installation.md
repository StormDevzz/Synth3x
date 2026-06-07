# Gentoo Linux: Guía de Instalación Paso a Paso

La instalación de Gentoo Linux es un proceso manual que implica configurar el almacenamiento, extraer los archivos base, entrar mediante chroot en el entorno de destino y compilar el kernel y el gestor de arranque (bootloader).

---

## Fases de la Instalación

1.  **Iniciar el Entorno Live:** Arranque la ISO de Synth3x o un CD de instalación mínimo.
2.  **Configurar la Red:** Establezca conexión a internet para descargar paquetes y archivos del sistema.
3.  **Preparar el Almacenamiento:**
    *   Cree una tabla de particiones GPT o MBR.
    *   Cree las particiones necesarias: EFI (boot) y raíz (ext4/btrfs).
    *   Formatee y monte las particiones bajo `/mnt/gentoo`.
4.  **Extraer Stage3:** Descargue y descomprima la imagen base Stage3 correspondiente a su perfil.
5.  **Entrar al entorno mediante Chroot:**
    *   Monte los sistemas de archivos virtuales (`/proc`, `/sys`, `/dev`).
    *   Ejecute: `chroot /mnt/gentoo /bin/bash`.
6.  **Configurar Portage:** Sincronice el repositorio y configure las flags en `/etc/portage/make.conf`.
7.  **Instalar el Gestor de Arranque:** Despliegue GRUB, configure los usuarios y reinicie el sistema.
