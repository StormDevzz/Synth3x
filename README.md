<p align="center">
  <img src="https://i.imgur.com/aw5D7My.png" alt="S3n">
</p>

# Synth3x

**Gentoo Hardened Desktop** — Wayland, Tor, full-disk encryption, terminal installer.

---

## Quick Start

```bash
make all              # Build everything
make run              # QEMU (kernel + initramfs)
make run-iso          # Boot from ISO
make run-installer    # ISO in installer mode
```

Build a full ISO:

```bash
./scripts/build_iso.sh
```

---

## Desktop

Custom Wayland compositor with DRM/KMS — no X11.

| Key | Action |
|-----|--------|
| `1` | New window |
| `CapsLock` | Close window |
| `Tab` | Switch window |
| `Up` / `Dn` | Switch workspace |
| `ESC` | Exit |

Built-in apps: Terminal, Browser, SysInfo, 8-page Guide.

---

## Commands

| What | How |
|------|-----|
| Web | `browser` |
| Install packages | `syn inst <pkg>` / `syn binary <pkg>` |
| List installed | `syn list` |
| Search | `syn search <query>` |
| Install OS | `synth3x-installer` |
| WiFi setup | `synth3x-wifi <SSID> <password>` |
| Download stage3 | `synth3x-downloader --stage3` |
| System info | `SysInfo` desktop window |
| Portage directly | `emerge --ask <pkg>` |

---

## Internet

- **Ethernet** — auto DHCP
- **WiFi** — `synth3x-wifi MySSID MyPassword`
- **Interactive** — `iwctl` or `wpa_supplicant`
- **Static IP** — `ip addr add` + `ip route add default`
- **Tor** — transparent proxy with nftables firewall

---

## Installer (10 steps)

1. Safety check — boot disk detection
2. WiFi / network setup
3. User account creation
4. Storage scan
5. Triple-confirmation safety check
6. Desktop selection (AmnesiaDE / KDE / GNOME)
7. Download Stage3 + Portage
8. GPT partitioning (EFI + ext4)
9. Gentoo base installation + chroot
10. Desktop deployment + GRUB

---

## GRUB Boot Menu

| Entry | Description |
|-------|-------------|
| Synth3x (Desktop) | Graphical desktop |
| Synth3x Installer | Installer environment |
| Reboot | Restart |
| Shutdown | Power off |

---

## Technology

| Component | Stack |
|-----------|-------|
| Compositor | C + Assembly, Wayland, DRM/KMS |
| Init | C, hardware detection |
| Installer | Rust + C |
| Package manager | C (`syn`) + Portage/emerge |
| WiFi | C (wpa_supplicant / iwd / nmcli) |
| Downloader | C (curl / wget / busybox) |
| Security | Rust — process supervision, privilege separation |
| Build | GNU Make + Cargo + grub-mkrescue |

---

## Hardware Support

Auto-detection for Lenovo, Acer, Dell, HP laptops.
