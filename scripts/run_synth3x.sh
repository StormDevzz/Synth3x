#!/bin/bash
# Synth3x-Anon VM Launcher
DIR="$(cd "$(dirname "$0")/.." && pwd)"
ISO="$DIR/iso/synth3x-anon.iso"

if [ ! -f "$ISO" ]; then
    echo "ISO not found. Run 'bash scripts/build_anon_iso.sh' first."
    exit 1
fi

echo "Starting Synth3x-Anon v0.8 VM..."
echo "  CPU: host (KVM), RAM: 1024M"
echo "  Graphics: std VGA + usb-tablet (mouse works without grab)"
echo "  Network: user mode (NAT)"
echo ""
echo "  TIPS:"
echo "  - Click inside window to capture keyboard"
echo "  - Mouse works immediately (no grab needed)"
echo "  - Ctrl+Alt+G to release capture"
echo ""

exec qemu-system-x86_64 \
    -cdrom "$ISO" \
    -m 1024 \
    -cpu host \
    -accel kvm \
    -vga std \
    -usb \
    -device usb-tablet \
    -netdev user,id=net0 \
    -device virtio-net,netdev=net0 \
    -audiodev none,id=none \
    -display gtk
