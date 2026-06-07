#!/bin/bash
# Synth3x OS — build script
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

echo "  ╔══════════════════════════════════════╗"
echo "  ║     Synth3x OS — Build System        ║"
echo "  ║     Pure C  |  Assembly  |  GRUB     ║"
echo "  ╚══════════════════════════════════════╝"

# Check dependencies
echo "[1/4] Checking dependencies..."
for cmd in gcc ld grub-mkrescue xorriso mtools qemu-system-x86_64; do
    which "$cmd" >/dev/null 2>&1 && echo "  ✓ $cmd" || echo "  ⚠ $cmd not found"
done

# Build
echo "[2/4] Building kernel + DE..."
make clean 2>/dev/null
make -j$(nproc) all 2>&1

# Create ISO
echo "[3/4] Creating ISO..."
make iso 2>&1

# Done
if [ -f iso/synth3x.iso ]; then
    echo ""
    echo "  ── Synth3x OS ISO ready ──"
    echo "  File: iso/synth3x.iso"
    echo "  Size: $(du -h iso/synth3x.iso | cut -f1)"
    echo ""
    echo "  Run in QEMU:"
    echo "    qemu-system-x86_64 -cdrom iso/synth3x.iso -m 512 -accel kvm"
    echo ""
    echo "  Write to USB:"
    echo "    dd if=iso/synth3x.iso of=/dev/sdX bs=4M status=progress"
    echo ""
else
    echo "  [✗] ISO creation failed."
fi
