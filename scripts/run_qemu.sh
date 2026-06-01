#!/bin/bash
# Synth3x-Anon v0.8 — QEMU Launcher (with Touchpad, Browser, HDD support)
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

ISO_PATH="iso/synth3x-anon.iso"
DISK_PATH="build/synth3x-anon.qcow2"

if [ ! -f "$ISO_PATH" ]; then
    echo "  [✗] ISO not found: $ISO_PATH"
    echo "      Run './scripts/build_anon_iso.sh' first."
    exit 1
fi

echo "  ╔══════════════════════════════════════════════════════════╗"
echo "  ║   Synth3x-Anon v0.8 — QEMU Virtual Machine              ║"
echo "  ║   Memory: 1024MB | VGA: std | Graphics: bochs-drm       ║"
echo "  ║   Mouse: usb-tablet (works without grab)                 ║"
echo "  ║   Touchpad: auto-detect | Browser: w3m | syn pkg mgr   ║"
echo "  ╚══════════════════════════════════════════════════════════╝"
echo ""

# Create virtual disk for HDD install if not exists
if [ ! -f "$DISK_PATH" ]; then
    echo "  -- Creating 32GB QCOW2 virtual disk for HDD install..."
    mkdir -p build
    qemu-img create -f qcow2 "$DISK_PATH" 32G >/dev/null
    echo "  ✓ Virtual disk: $DISK_PATH"
    echo ""
fi

# Check KVM
KVM_ACCEL="-accel kvm -cpu host"
if ! [ -w /dev/kvm ]; then
    echo "  ⚠ KVM not available. Running without acceleration."
    KVM_ACCEL="-cpu Penryn"
fi

echo "  Booting Synth3x-Anon v0.8..."
echo "  Features: syn, w3m browser, touchpad support, Gentoo installer"
echo "  TIP: Click inside VM window to capture keyboard"
echo "       Mouse works immediately (no grab needed)"
echo ""

# Launch with usb-tablet for mouse (no grab needed) + std VGA (bochs driver works)
qemu-system-x86_64 \
    -cdrom "$ISO_PATH" \
    -drive file="$DISK_PATH",if=virtio \
    -m 1024 \
    $KVM_ACCEL \
    -vga std \
    -usb \
    -device usb-tablet \
    -net nic,model=virtio \
    -net user,hostfwd=tcp::2222-:22 \
    -audiodev none,id=none \
    -display gtk

echo ""
echo "  VM stopped."
