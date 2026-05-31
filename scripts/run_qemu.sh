#!/bin/bash
# Synth3x-Anon — QEMU Launch Script
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

ISO_PATH="iso/synth3x-anon.iso"
DISK_PATH="build/synth3x-anon.qcow2"

if [ ! -f "$ISO_PATH" ]; then
    echo "  [✗] ISO not found: $ISO_PATH"
    echo "      Please run './scripts/build_anon_iso.sh' first to compile the distribution."
    exit 1
fi

echo "  ╔══════════════════════════════════════════════════════════╗"
echo "  ║        Synth3x-Anon — Booting Virtual Machine            ║"
echo "  ║        Memory: 1024MB | VGA: std | Net: User-Mode        ║"
echo "  ╚══════════════════════════════════════════════════════════╝"
echo ""

# Automatically check and create the virtual hard disk for real installations
if [ ! -f "$DISK_PATH" ]; then
    echo "  -- Virtual disk image not found at $DISK_PATH"
    echo "  -- Automatically creating a new 20GB QCOW2 virtual disk..."
    mkdir -p build
    qemu-img create -f qcow2 "$DISK_PATH" 20G >/dev/null
    echo "  ✓ Virtual disk created successfully!"
    echo ""
fi

# Check KVM availability
KVM_ACCEL="-accel kvm -cpu host"
if ! [ -w /dev/kvm ]; then
    echo "  ⚠ /dev/kvm is not writable. Running QEMU without KVM acceleration (slower)..."
    KVM_ACCEL="-cpu Penryn"
fi

# Locate UEFI firmware (OVMF) on the host for QEMU UEFI boot
UEFI_BIOS=""
for path in \
    /usr/share/edk2/x64/OVMF.4m.fd \
    /usr/share/edk2/x64/OVMF_CODE.4m.fd \
    /usr/share/edk2-ovmf/x64/OVMF_CODE.fd \
    /usr/share/OVMF/OVMF_CODE.fd \
    /usr/share/ovmf/x64/OVMF.fd \
    /usr/share/qemu/edk2-x86_64-code.fd \
    /usr/share/ovmf/OVMF.fd \
    /usr/share/edk2/x64/OVMF.fd; do
    if [ -f "$path" ]; then
        UEFI_BIOS="-bios $path"
        break
    fi
done

if [ -z "$UEFI_BIOS" ]; then
    echo "  ⚠ UEFI firmware (OVMF) not found on host."
    echo "    QEMU will boot Live ISO in Legacy mode."
fi

# Run QEMU with the virtual disk, virtio network card and user networking
qemu-system-x86_64 \
    $UEFI_BIOS \
    -cdrom "$ISO_PATH" \
    -drive file="$DISK_PATH",if=virtio \
    -m 1024 \
    $KVM_ACCEL \
    -vga std \
    -device usb-ehci -device usb-tablet \
    -net nic,model=virtio \
    -net user \
    -serial stdio
