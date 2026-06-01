# Synth3x OS v0.8 — Gentoo Hardened Desktop

Desktop OS written in C and Assembly. Custom framebuffer DE with:
- **Browser** (w3m), **Touchpad** support, **syn** package manager
- **Networking via Tor** transparent proxy + nftables firewall
- **Hardware detection** for Lenovo, Acer, Dell, HP laptops
- **Optional hard disk install** (GRUB, UEFI)

## Build

```bash
./scripts/build_anon_iso.sh    # Full ISO with all features
```

## Run in QEMU

```bash
make run                        # Quick QEMU (ISO boot)
./scripts/run_qemu.sh           # Full QEMU with disk + touchpad
```

## Keys

`1` new window · `CapsLock` close · `Tab` switch · `Up/Dn` workspace · `ESC` exit

## Features

| Feature | Command |
|---------|---------|
| Web Browser | `browser` in Terminal |
| Package Manager | `syn inst`/`syn binary`/`syn list` |
| Install to HDD | Press ESC, run: `synth3x-installer` |
| Hardware Info | `SysInfo` window on desktop |
| Guide | `Synth3x Guide` — 6 pages (network, touchpad, install) |

## Internet Setup

1. **Ethernet**: Open Guide → Page 1 → Click `[SETUP INTERNET]`
2. **WiFi**: `iwctl station wlan0 connect "SSID"`
3. **Static IP**: `ip addr add` and `ip route add default`

## syn Package Manager (Gentoo-style)

```
sudo syn inst telegram-desktop  # Install binary package
sudo syn binary vscodium        # Binary install
syn list                        # List installed packages
syn search firefox              # Search packages
```

## Directory Structure

```
src/hardware/  — Assembly CPUID + C hardware detection
src/init/      — PID 1 (Gentoo-style boot with HW detect)
src/synth3x/   — AmnesiaDE framebuffer desktop (browser, lock)
src/commands/  — syn, reboot, shutdown
src/who/       — System analyzers (ram, disk, usb, net)
boot/          — GRUB config, nftables, torrc
scripts/       — Build, installer, QEMU launcher
```
