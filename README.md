                           ╔══════════════════════╗
                           ║  S Y N T H 3 X     ║
                           ║       O S          ║
                           ║  "Pure C. Pure C." ║
                           ╚══════════════════════╝

Synth3x OS — desktop operating system written in pure C and Assembly.

- **Synth3x DE** — custom desktop environment (framebuffer compositor)
- **Kernel** — multiboot2-compliant x86 kernel (C + ASM)
- **Init** — userspace environment selector (Synth3x DE / Xfce / Shell)
- **Time-based theming** — adapts to time of day (morning/day/evening/night)
- **Notifications** — custom notification system via FIFO
- **Double-buffered rendering** — smooth 60fps framebuffer drawing
- **No X11, No Wayland, No GTK, No bloat**

## Build

```bash
git clone https://github.com/StormDevzz/Synth3x
cd Synth3x

# Custom multiboot kernel
make

# Or full Linux-based live ISO
./scripts/build.sh
```

## Run in QEMU

```bash
qemu-system-x86_64 -cdrom iso/synth3x-os.iso -m 1024 -accel kvm -vga std
```

## Keyboard shortcuts

| Key | Action |
|------|--------|
| `1` | New window |
| `CapsLock` | Close active window |
| `Tab` | Switch windows |
| `Up/Down` | Switch workspace |
| `ESC` | Exit to shell |

## Project structure

```
src/
  kernel/       Multiboot kernel (C + ASM)
  init/         Userspace init (C)
  synth3x/      Synth3x DE compositor (C)
build/          Compiled binaries
iso/            Bootable ISO images
boot/           GRUB configuration
```

## License

MIT — do what you want.
