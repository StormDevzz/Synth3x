# Synth3x OS

Desktop OS written in C and Assembly. Custom kernel, init, and synthesizer-style desktop environment. No X11, Wayland, or GTK.

## Build

```bash
make              # kernel + init + synth3x DE
./scripts/build.sh  # full live ISO
```

## Run

```bash
qemu-system-x86_64 -cdrom iso/synth3x-os.iso -m 1024 -accel kvm
```

## Keys

`1` new window · `CapsLock` close · `Tab` switch · `Up/Dn` workspace · `ESC` exit
