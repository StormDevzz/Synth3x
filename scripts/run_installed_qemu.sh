#!/bin/bash
# Synth3x — Boot Installed OS from Virtual Disk
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

DISK_PATH="build/synth3x.qcow2"

if [ ! -f "$DISK_PATH" ]; then
    echo "  [✗] Virtual disk not found: $DISK_PATH"
    echo "      Please run the installation inside QEMU first."
    exit 1
fi

echo "  ╔══════════════════════════════════════════════════════════╗"
echo "  ║        Synth3x — Booting Installed Gentoo OS             ║"
echo "  ║        Boot Device: Virtual Hard Drive (/dev/vda)        ║"
echo "  ║        Memory: 1024MB | VGA: std | Net: User-Mode        ║"
echo "  ╚══════════════════════════════════════════════════════════╝"
echo ""

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
    echo "    QEMU will boot in Legacy mode (might fail to boot from UEFI partition)."
fi

# Launch QEMU booting directly from the hard disk (without CD-ROM attached)
qemu-system-x86_64 \
    $UEFI_BIOS \
    -drive file="$DISK_PATH",if=virtio \
    -m 1024 \
    $KVM_ACCEL \
    -vga std \
    -device usb-ehci -device usb-tablet \
    -net nic,model=virtio \
    -net user \
    -serial stdio
