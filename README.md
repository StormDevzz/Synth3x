# Synth3x-Anon OS v0.8.1 Beta — Gentoo Hardened Desktop

Desktop OS written in C, Assembly, and Rust. Custom Wayland compositor with:
- **AmnesiaDE** — Custom Wayland compositor (DRM/KMS, no X11)
- **Browser** (w3m), **Touchpad** support, **syn** package manager
- **Networking via Tor** transparent proxy + nftables firewall
- **Hardware detection** for Lenovo, Acer, Dell, HP laptops
- **Terminal Installer** — Full Gentoo installation with WiFi support
- **Portage/emerge** — Default package manager (Gentoo-compatible)

## Build

```bash
./scripts/build_anon_iso.sh    # Full ISO with all features
make all                       # Build all components
make iso                       # Build ISO only
```

## Run in QEMU

```bash
make run                        # Quick QEMU (kernel + initramfs)
make run-iso                    # Boot from ISO in QEMU
make run-installer              # Boot ISO in installer mode
./scripts/run_qemu.sh           # Full QEMU with disk + touchpad
```

## Keys

`1` new window · `CapsLock` close · `Tab` switch · `Up/Dn` workspace · `ESC` exit

## Features

| Feature | Command |
|---------|---------|
| Web Browser | `browser` in Terminal |
| Package Manager | `syn inst`/`syn binary`/`syn list` |
| Install to HDD | `synth3x-installer` in terminal |
| WiFi Setup | `synth3x-wifi <SSID> <password>` |
| Download Files | `synth3x-downloader --stage3` |
| Hardware Info | `SysInfo` window on desktop |
| Guide | `Synth3x Guide` — 8 pages |

## GRUB Boot Menu

| Entry | Description |
|-------|-------------|
| Synth3x-Anon v0.8.1 (AmnesiaDE) | Boot into graphical desktop |
| Synth3x-Anon v0.8.1 Installer | Boot into installer environment |
| Reboot | Restart system |
| Shutdown | Power off |

## Internet Setup

1. **Ethernet**: Auto-configured via DHCP
2. **WiFi**: `synth3x-wifi MyNetwork MyPassword`
3. **Interactive**: `iwctl` (iwd) or `wpa_supplicant`
4. **Static IP**: `ip addr add` and `ip route add default`

## Installation Process

The installer runs 10 steps:
1. Safety check (boot disk detection)
2. **WiFi/Network setup** (connect to internet)
3. User account provisioning
4. Storage topology scan
5. Safety verification (triple confirmation + countdown)
6. Desktop environment selection (AmnesiaDE/KDE/GNOME)
7. Download installation files (Stage3, Portage)
8. GPT partitioning (EFI + root ext4)
9. Gentoo base system installation (Stage3 + chroot)
10. Desktop environment deployment + GRUB bootloader

## syn Package Manager (Gentoo-compatible)

```bash
sudo syn inst telegram-desktop  # Install binary package
sudo syn binary vscodium        # Binary install
syn list                        # List installed packages
syn search firefox              # Search packages
emerge --ask <package>          # Use Portage directly
```

## Architecture

```
src/compositor/   — AmnesiaDE Wayland compositor (DRM/KMS)
src/init/         — PID 1 (Gentoo-style boot with HW detect)
src/installer/    — C downloader + WiFi manager for installer
src/commands/     — syn, reboot, shutdown
src/who/          — System analyzers (ram, disk, usb, net)
src/lib/          — Rust safe process foundation + installer
boot/             — GRUB config, nftables, torrc
scripts/          — Build, QEMU launcher
```

## Technology Stack

| Component | Technology |
|-----------|-----------|
| Compositor | C + Assembly (Wayland, DRM/KMS) |
| Init system | C (PID 1, hardware detection) |
| Installer | Rust + C + Assembly |
| Package manager | C (syn) + Portage/emerge |
| WiFi manager | C (wpa_supplicant/iwctl/nmcli) |
| File downloader | C (curl/wget/busybox) |
| Security | Rust (process supervision, privilege separation) |
| Build | GNU Make + Cargo + grub-mkrescue |
