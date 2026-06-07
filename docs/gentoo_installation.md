# Gentoo Linux: Step-by-Step Installation Overview

Installing Gentoo Linux is a manual process that involves configuring storage, extracting base files, chrooting into the target environment, and building the kernel/bootloader.

---

## The Installation Pipeline

1.  **Boot the Live Environment:** Boot a minimal installation CD or a live environment like Synth3x.
2.  **Configure Network:** Establish internet connectivity to fetch packages and files.
3.  **Prepare Storage:**
    *   Create a GPT or MBR partition table.
    *   Create partitions: EFI (boot) and Root (ext4/btrfs).
    *   Format and mount the partitions under `/mnt/gentoo`.
4.  **Extract Stage3:** Download and unpack a Stage3 archive corresponding to your profile.
5.  **Chroot into the New System:**
    *   Mount host virtual filesystems (`/proc`, `/sys`, `/dev`).
    *   Enter the environment: `chroot /mnt/gentoo /bin/bash`.
6.  **Configure Portage:** Sync the repository and configure compile flags in `/etc/portage/make.conf`.
7.  **Build the Kernel:** Configure and compile the Linux kernel.
8.  **Setup System Configs:** Define `/etc/fstab`, hostname, locales, and network configuration.
9.  **Install Bootloader:** Deploy GRUB and reboot into your fresh Gentoo system!
