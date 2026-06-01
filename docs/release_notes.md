# Synth3x-Anon v0.8 — Gentoo Hardened OS
# Browser | Touchpad | syn pkg Manager | HW Detect

================================================================
                    INTERNET SETUP GUIDE
================================================================

## 1. WIRED (Ethernet) — Auto

1. Plug in Ethernet cable
2. In the DE, open "Synth3x Guide" window → Page 1
3. Click "[ SETUP INTERNET ]" button — DHCP auto-configures
4. Or in Terminal: # dhcpcd eth0

## 2. WI-FI Connection

Method A — iwd (recommended):
  # iwctl station wlan0 scan
  # iwctl station wlan0 get-networks
  # iwctl station wlan0 connect "SSID"

Method B — wpa_supplicant:
  # wpa_passphrase "SSID" password > /etc/wpa.conf
  # wpa_supplicant -B -i wlan0 -c /etc/wpa.conf
  # udhcpc -i wlan0

## 3. Static IP
  # ip addr add 192.168.1.100/24 dev eth0
  # ip route add default via 192.168.1.1
  # echo 'nameserver 1.1.1.1' > /etc/resolv.conf

## 4. USB Tethering
  Connect phone via USB, enable USB tethering.
  # dhcpcd usb0

================================================================
                    DESKTOP ENVIRONMENT (AmnesiaDE v0.8)
================================================================

Pre-installed apps on desktop:
  • Terminal     — type commands, run 'browser' for web
  • SysInfo      — hardware stats (CPU, RAM, disk, Tor)
  • Web          — w3m terminal browser
  • Handbook     — DE documentation
  • Guide        — 6-page setup guide (network, touchpad, install)
  • Install      — hard disk installer

Keyboard shortcuts:
  Super+1..4  — Switch workspaces
  Tab         — Cycle windows
  CapsLock    — Close window
  Up/Down     — Previous/next workspace
  ESC         — Exit DE to terminal

================================================================
                    TOUCHPAD SUPPORT
================================================================

Supported laptops:
  • Lenovo (ThinkPad, IdeaPad)
  • Acer (Aspire, Predator, Swift)
  • Dell (XPS, Inspiron, Latitude)
  • HP (Pavilion, Envy, Spectre)
  • ASUS (ROG, ZenBook, VivoBook)
  • Apple MacBook

Touchpad drivers auto-loaded. Two-finger scroll supported.
Status shown in panel: "TP: ON"

================================================================
                    syn PACKAGE MANAGER (Gentoo-style)
================================================================

Usage:
  sudo syn inst <package>     Install package (binary)
  sudo syn binary <package>   Install binary directly
  sudo syn remove <package>   Remove package
  syn list                    List installed packages
  syn search <query>          Search packages
  syn update                  Update package database
  syn info <package>          Package details

Examples:
  sudo syn inst telegram-desktop
  sudo syn binary firefox
  sudo syn inst vscodium
  sudo syn list

Available packages:
  telegram-desktop, firefox, vscodium, vim, htop,
  neofetch, git, wget, nano, gcc, python, nodejs,
  rustc, go, nginx, docker, ibus

================================================================
                    HARD DISK INSTALLATION
================================================================

1. In the DE (Live CD), press ESC to exit to terminal
2. Run: # synth3x-installer
3. Enter username and sudo password when prompted
4. Select target drive
5. Choose Desktop Environment
6. The installer partitions, formats, copies files,
   and installs GRUB bootloader
7. Reboot and remove CD — boot from hard disk!

Boot from HDD is supported for both BIOS and UEFI.

================================================================
                    BUILD FROM SOURCE
================================================================

Requirements: gcc, ld, grub-mkrescue, xorriso, mtools,
              tor, nft, busybox

  ./scripts/build_anon_iso.sh

Run in QEMU:
  qemu-system-x86_64 -cdrom iso/synth3x-anon.iso -m 1024 -accel kvm

================================================================
                    HARDWARE DETECTION
================================================================

Synth3x auto-detects at boot:
  • CPU vendor (Intel/AMD) via CPUID assembly
  • Laptop model via DMI/SMBIOS
  • Touchpad type (Synaptics, ALPS, Elan)
  • Network interfaces
  • USB devices

Assembly routines in src/hardware/hw_cpuid.S:
  • hw_cpuid_vendor — get CPU vendor string
  • hw_cpuid_features — CPU feature bitmask
  • hw_cpuid_brand_string — full CPU brand name
