#!/bin/bash
# Synth3x-Anon — Automated Live ISO Builder
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

echo "  ╔══════════════════════════════════════════════════════════╗"
# Vibrant cyan/blue CLI headers
echo "  ║        Synth3x-Anon — Secure & Amnesic Build System       ║"
echo "  ║        Tor Routing | nftables Firewall | Amnesic RAM     ║"
echo "  ╚══════════════════════════════════════════════════════════╝"

# 1. Dependency Check
echo "[1/5] Checking build dependencies..."
for cmd in gcc ld grub-mkrescue xorriso mtools tor nft busybox; do
    if which "$cmd" >/dev/null 2>&1; then
        echo "  ✓ $cmd found"
    else
        echo "  ⚠ $cmd not found in system PATH"
    fi
done

# 2. Build kernel helper elements & userspace
echo "[2/5] Compiling custom PID 1 init & Synth3x DE..."
make clean 2>/dev/null || true
mkdir -p build

gcc -static -march=x86-64 -mno-avx -O2 -Wall -o build/init src/init/init.c src/init/splash.S src/synth3x/font.S -lpthread -lrt
echo "  ✓ Init (PID 1) compiled successfully"

gcc -static -march=x86-64 -mno-avx -O2 -Wall -o build/synth3x src/synth3x/synth3x.c src/synth3x/font.S -lpthread -lrt
echo "  ✓ Synth3x DE compiled successfully"

echo "  -- Compiling security checks..."
mkdir -p build/checks
gcc -static -march=x86-64 -mno-avx -O2 -o build/checks/check_tor src/checks/check_tor.c
gcc -static -march=x86-64 -mno-avx -O2 -o build/checks/check_firewall src/checks/check_firewall.c
gcc -static -march=x86-64 -mno-avx -O2 -o build/checks/check_mac src/checks/check_mac.c
gcc -static -march=x86-64 -mno-avx -O2 -o build/checks/check_amnesic src/checks/check_amnesic.c
gcc -static -march=x86-64 -mno-avx -O2 -o build/checks/check_hostname src/checks/check_hostname.c

echo "  -- Compiling custom native commands..."
mkdir -p build/commands
gcc -static -march=x86-64 -mno-avx -O2 -o build/commands/reboot src/commands/reboot.c
gcc -static -march=x86-64 -mno-avx -O2 -o build/commands/shutdown src/commands/shutdown.c

echo "  -- Compiling C hardware analyzers..."
gcc -static -march=x86-64 -mno-avx -O2 -Wall -o build/ram_analyzer src/who/ram_analyzer.c
gcc -static -march=x86-64 -mno-avx -O2 -Wall -o build/disk_analyzer src/who/disk_analyzer.c
gcc -static -march=x86-64 -mno-avx -O2 -Wall -o build/device_names src/who/device_names.c
gcc -static -march=x86-64 -mno-avx -O2 -Wall -o build/usb_analyzer src/who/usb_analyzer.c
gcc -static -march=x86-64 -mno-avx -O2 -Wall -o build/cable_analyzer src/who/cable_analyzer.c

# 3. Create isolated initramfs structure
echo "[3/5] Constructing RAM-only amnesic initramfs..."
INITRAMFS_DIR="build/initramfs"
rm -rf "$INITRAMFS_DIR"
mkdir -p "$INITRAMFS_DIR"/{bin,sbin,usr/bin,usr/sbin,dev,proc,sys,tmp,etc,var,lib,lib64,usr/lib,usr/lib64}

# Copy our compiled custom software
cp build/init "$INITRAMFS_DIR/init"
cp build/synth3x "$INITRAMFS_DIR/usr/bin/synth3x"
cp scripts/synth3x-installer.sh "$INITRAMFS_DIR/usr/bin/synth3x-installer"

# Copy hardware analyzers compiled from who/
cp build/ram_analyzer "$INITRAMFS_DIR/usr/bin/"
cp build/disk_analyzer "$INITRAMFS_DIR/usr/bin/"
cp build/device_names "$INITRAMFS_DIR/usr/bin/"
cp build/usb_analyzer "$INITRAMFS_DIR/usr/bin/"
cp build/cable_analyzer "$INITRAMFS_DIR/usr/bin/"

# Bundle AmnesiaIDE development environment files
mkdir -p "$INITRAMFS_DIR/AmnesiaIDE"
cp -r AmnesiaIDE/* "$INITRAMFS_DIR/AmnesiaIDE/"

# Copy security checks C binaries
mkdir -p "$INITRAMFS_DIR/usr/bin/checks"
cp build/checks/* "$INITRAMFS_DIR/usr/bin/checks/"

# Create security checks auditor script
cat << 'EOF' > "$INITRAMFS_DIR/usr/bin/checks-all"
#!/bin/bash
# Synth3x-Anon Security Auditor
CYAN='\033[0;36m'
LIGHT_CYAN='\033[1;36m'
PURPLE='\033[0;35m'
GREEN='\033[0;32m'
NC='\033[0m'

echo -e "${CYAN}  ╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}  ║${NC}        ${PURPLE}Synth3x-Anon Security Auditor & Integrity Checks${NC}     ${NC}${CYAN}║${NC}"
echo -e "${CYAN}  ╚══════════════════════════════════════════════════════════╝${NC}"
echo ""

/usr/bin/checks/check_tor
/usr/bin/checks/check_firewall
/usr/bin/checks/check_mac
/usr/bin/checks/check_amnesic
/usr/bin/checks/check_hostname

echo ""
echo -e "${GREEN}[+] Audit complete! System integrity verified.${NC}"
EOF
chmod +x "$INITRAMFS_DIR/usr/bin/checks-all"

# Find paths for busybox, tor, and nft
BUSYBOX_BIN=$(which busybox 2>/dev/null || echo "/usr/lib/initcpio/busybox")
TOR_BIN=$(which tor 2>/dev/null || echo "/usr/bin/tor")
NFT_BIN=$(which nft 2>/dev/null || echo "/usr/sbin/nft")

if [ ! -f "$BUSYBOX_BIN" ]; then
    echo "  [✗] busybox binary not found at $BUSYBOX_BIN"
    exit 1
fi

cp "$BUSYBOX_BIN" "$INITRAMFS_DIR/bin/busybox"
cp "$TOR_BIN" "$INITRAMFS_DIR/usr/bin/tor" 2>/dev/null || true
cp "$NFT_BIN" "$INITRAMFS_DIR/usr/sbin/nft" 2>/dev/null || true
cp /bin/bash "$INITRAMFS_DIR/bin/bash"

# Populate all BusyBox applets automatically (gives us grep, awk, sed, etc. for free!)
cd "$INITRAMFS_DIR"
ln -sf busybox bin/sh
./bin/busybox --install -s bin/ 2>/dev/null || true

# Copy compiled custom C commands (reboot and shutdown) directly to bin/
rm -f bin/reboot bin/shutdown bin/poweroff
cp "$DIR/build/commands/reboot" bin/reboot
cp "$DIR/build/commands/shutdown" bin/shutdown
cp "$DIR/build/commands/shutdown" bin/poweroff

# Create symlinks in sbin, usr/bin, and usr/sbin for universal command resolution
rm -f sbin/reboot sbin/shutdown sbin/poweroff
ln -sf /bin/reboot sbin/reboot
ln -sf /bin/shutdown sbin/shutdown
ln -sf /bin/poweroff sbin/poweroff

rm -f usr/bin/reboot usr/bin/shutdown usr/bin/poweroff
ln -sf /bin/reboot usr/bin/reboot
ln -sf /bin/shutdown usr/bin/shutdown
ln -sf /bin/poweroff usr/bin/poweroff

rm -f usr/sbin/reboot usr/sbin/shutdown usr/sbin/poweroff
ln -sf /bin/reboot usr/sbin/reboot
ln -sf /bin/shutdown usr/sbin/shutdown
ln -sf /bin/poweroff usr/sbin/poweroff

cd "$DIR"

# 4. Extract and copy dynamic libraries (so dynamic binaries run perfectly inside initramfs)
copy_deps() {
    local bin="$1"
    local dest="$2"
    echo "  -- Resolving dependencies for $(basename "$bin")..."
    ldd "$bin" 2>/dev/null | grep -o '/[^ ]\+' | while read -r lib; do
        if [ -f "$lib" ]; then
            local libdir=$(dirname "$lib")
            mkdir -p "${dest}${libdir}"
            cp -n "$lib" "${dest}${lib}" 2>/dev/null || true
        fi
    done
}

# Find and copy partitioning, formatting and network tools from the host
echo "  -- Scanning and copying host system utilities..."
for tool in parted fdisk mkfs.ext4 mkfs.vfat curl tar grep awk udevadm grub-install; do
    TOOL_PATH=$(which $tool 2>/dev/null)
    if [ -n "$TOOL_PATH" ] && [ -f "$TOOL_PATH" ]; then
        tool_dir=$(dirname "$TOOL_PATH")
        mkdir -p "$INITRAMFS_DIR$tool_dir"
        cp -p "$TOOL_PATH" "$INITRAMFS_DIR$TOOL_PATH" 2>/dev/null || true
        copy_deps "$TOOL_PATH" "$INITRAMFS_DIR"
    fi
done

# Copy GRUB x86_64-efi modules from the host to allow UEFI grub-install inside Live OS
if [ -d "/usr/lib/grub/x86_64-efi" ]; then
    echo "  -- Copying GRUB UEFI modules..."
    mkdir -p "$INITRAMFS_DIR/usr/lib/grub"
    cp -a "/usr/lib/grub/x86_64-efi" "$INITRAMFS_DIR/usr/lib/grub/"
fi

copy_deps "$TOR_BIN" "$INITRAMFS_DIR"
copy_deps "$NFT_BIN" "$INITRAMFS_DIR"
copy_deps "$INITRAMFS_DIR/usr/bin/synth3x"
copy_deps "/bin/bash" "$INITRAMFS_DIR"
copy_deps "$BUSYBOX_BIN" "$INITRAMFS_DIR"

# Ensure dynamic linkers are present in the lib directories
cp -aL /lib64/ld-linux-x86-64.so.* "$INITRAMFS_DIR/lib64/" 2>/dev/null || true
cp -aL /lib/ld-linux-x86-64.so.* "$INITRAMFS_DIR/lib/" 2>/dev/null || true

# Copy Anonymity and Firewall configurations
echo "  -- Integrating Tor & Firewall configuration files..."
mkdir -p "$INITRAMFS_DIR/etc/tor"
cp boot/torrc "$INITRAMFS_DIR/etc/tor/torrc"
cp boot/nftables.rules "$INITRAMFS_DIR/etc/nftables.rules"

# 5. Compress initramfs & Build Live ISO
echo "[4/5] Packaging compressed initramfs..."
cd "$INITRAMFS_DIR"
find . | cpio -H newc -o --quiet | gzip -9 -f > ../initrd.img
cd "$DIR"
echo "  ✓ Compressed initramfs size: $(du -h build/initrd.img | cut -f1)"

echo "[5/5] Creating bootable Live ISO..."
rm -rf iso
mkdir -p iso/boot/grub

if cp /boot/vmlinuz-linux iso/boot/vmlinuz-linux 2>/dev/null || cp /boot/vmlinuz* iso/boot/vmlinuz-linux 2>/dev/null; then
    echo "  ✓ Kernel copied from /boot"
else
    MOD_KERNEL=$(find /usr/lib/modules /lib/modules -name "vmlinuz" 2>/dev/null | head -n 1)
    if [ -n "$MOD_KERNEL" ] && cp "$MOD_KERNEL" iso/boot/vmlinuz-linux 2>/dev/null; then
        echo "  ✓ Kernel copied from modules fallback: $MOD_KERNEL"
    else
        echo "  ⚠ No host kernel found! Make sure to provide a kernel at iso/boot/vmlinuz-linux"
    fi
fi
cp build/initrd.img iso/boot/initrd.img
cp boot/grub.cfg iso/boot/grub/grub.cfg

if command -v grub-mkrescue >/dev/null 2>&1; then
    grub-mkrescue -o iso/synth3x-anon.iso iso -- -volid "SYNTH3X_ANON" >/dev/null 2>&1
    echo ""
    echo "  ── Synth3x-Anon ISO ready ──"
    echo "  File: iso/synth3x-anon.iso"
    echo "  Size: $(du -h iso/synth3x-anon.iso | cut -f1)"
    echo ""
    echo "  Run in QEMU with network routing:"
    echo "    qemu-system-x86_64 -cdrom iso/synth3x-anon.iso -m 1024 -accel kvm -net nic,model=virtio -net user"
    echo ""
else
    echo "  [✗] grub-mkrescue not found. Please install grub-mkrescue and xorriso to build the ISO."
fi
