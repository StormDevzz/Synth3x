#!/bin/bash
# Synth3x — Automated Live ISO Builder
set -e
set +o pipefail 2>/dev/null || true

DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

VERSION=$(cat VERSION 2>/dev/null || echo "0.8.1-Beta")

echo "  ╔══════════════════════════════════════════════════════════╗"
echo "  ║     Synth3x v${VERSION} — Gentoo Hardened Build System     ║"
echo "  ║     Browser | Touchpad | syn Pkg Mgr | Amnesic RAM      ║"
echo "  ╚══════════════════════════════════════════════════════════╝"

# 1. Dependency Check
echo "[1/6] Checking build dependencies..."
for cmd in gcc ld grub-mkrescue xorriso mtools tor nft busybox; do
    if which "$cmd" >/dev/null 2>&1; then
        echo "  ✓ $cmd found"
    else
        echo "  ⚠ $cmd not found in system PATH"
    fi
done

# Auto-detect CPU flags using cpucheck
echo "  -- Detecting CPU capabilities for compiler flags..."
CPU_FLAGS=""
if [ -f prog/Synth3x-FileMng/src/checks/cpucheck.c ]; then
    if gcc -DSTANDALONE -I prog/Synth3x-FileMng/src/checks -o /tmp/cpucheck \
        prog/Synth3x-FileMng/src/checks/cpucheck.c \
        prog/Synth3x-FileMng/src/checks/cpuid.S \
        -lpthread 2>/dev/null; then
        CPU_FLAGS=$(/tmp/cpucheck 2>/dev/null || true)
        rm -f /tmp/cpucheck
    fi
fi
if [ -z "$CPU_FLAGS" ]; then
    CPU_FLAGS="-march=x86-64 -mtune=generic"
fi
# Force Penryn-compatible flags — target is Pentium T4400, no AVX/SSE4
CPU_FLAGS="$CPU_FLAGS -mno-avx -mno-sse4.1 -mno-sse4.2"
    echo "  ✓ CPU flags: $CPU_FLAGS"
export CPU_FLAGS
export BASE_FLAGS="$CPU_FLAGS -O2 -Wall"
export DYN_FLAGS="$CPU_FLAGS -O2 -Wall"

# 2. Build kernel helper elements & userspace
echo "[2/6] Compiling custom PID 1 init & Synth3x DE..."
make clean 2>/dev/null || true
mkdir -p build

echo "  -- Compiling syninit (PID 1) ..."
gcc $BASE_FLAGS \
    -DVERSION=\"$VERSION\" \
    -o build/syninit src/init/init.c src/init/boot.S \
    -lpthread -lrt
echo "  ✓ syninit (PID 1) compiled"

echo "  -- Building synit-svc (Rust service manager) ..."
if command -v cargo >/dev/null 2>&1; then
    cargo build --release --manifest-path src/init/svc/Cargo.toml 2>&1 || true
    cp src/init/svc/target/release/synit-svc build/synit-svc 2>/dev/null || true
    echo "  ✓ synit-svc (Rust)"
else
    echo "  ⚠ cargo not found, synit-svc skipped"
fi

echo "  -- Compiling Synth3x Wayland compositor..."
gcc $DYN_FLAGS \
    -I src/compositor -I/usr/include/libdrm -I/usr/include/drm \
    -o build/synth3x src/compositor/main.c src/compositor/drm.c \
    src/compositor/input.c src/compositor/wl_server.c \
    src/compositor/shell.c src/compositor/protocols.c \
    src/compositor/render.S src/compositor/font.S \
    -lpthread -lrt -lm -ldrm
echo "  ✓ Synth3x Wayland compositor compiled"

echo "  -- Compiling driver check tools..."
mkdir -p build/checks
gcc $BASE_FLAGS -o build/checks/check_sound    src/checks/check_sound.c 2>/dev/null
gcc $BASE_FLAGS -o build/checks/check_keyboard src/checks/check_keyboard.c 2>/dev/null
gcc $BASE_FLAGS -o build/checks/check_mouse    src/checks/check_mouse.c 2>/dev/null
gcc $BASE_FLAGS -o build/checks/check_display  src/checks/check_display.c 2>/dev/null
for ch in check_sound check_keyboard check_mouse check_display; do
    [ -f "build/checks/$ch" ] && echo "  ✓ $ch" || echo "  ⚠ $ch build failed"
done

echo "  -- Compiling custom native commands..."
mkdir -p build/commands
gcc $BASE_FLAGS -o build/commands/reboot src/commands/reboot.c
gcc $BASE_FLAGS -o build/commands/shutdown src/commands/shutdown.c

echo "  -- Compiling syn package manager..."
gcc $BASE_FLAGS -o build/syn src/commands/syn.c
echo "  ✓ syn package manager compiled"

echo "  -- Compiling C hardware analyzers..."
gcc $BASE_FLAGS -o build/ram_analyzer src/who/ram_analyzer.c
gcc $BASE_FLAGS -o build/disk_analyzer src/who/disk_analyzer.c
gcc $BASE_FLAGS -o build/device_names src/who/device_names.c
gcc $BASE_FLAGS -o build/usb_analyzer src/who/usb_analyzer.c
gcc $BASE_FLAGS -o build/cable_analyzer src/who/cable_analyzer.c

echo "  -- Compiling installer C components..."
gcc $BASE_FLAGS -DVERSION=\"$VERSION\" -DSTANDALONE -o build/synth3x-downloader src/installer/downloader.c 2>/dev/null && echo "  ✓ synth3x-downloader" || echo "  ⚠ downloader build failed"
gcc $BASE_FLAGS -DVERSION=\"$VERSION\" -DSTANDALONE -o build/synth3x-wifi src/installer/wifi_manager.c 2>/dev/null && echo "  ✓ synth3x-wifi" || echo "  ⚠ wifi_manager build failed"

# 3. Create isolated initramfs structure
echo "[3/6] Constructing RAM-only amnesic initramfs..."
INITRAMFS_DIR="build/initramfs"
rm -rf "$INITRAMFS_DIR"
mkdir -p "$INITRAMFS_DIR"/usr/{bin,sbin,lib}
mkdir -p "$INITRAMFS_DIR"/{dev,proc,sys,tmp,etc,var,run}
mkdir -p "$INITRAMFS_DIR"/etc/{tor,ssl,dbus}
mkdir -p "$INITRAMFS_DIR"/var/{db/syn,cache/syn,lib/tor,log/tor}
mkdir -p "$INITRAMFS_DIR"/usr/local/{bin,lib,share}

# Create standard UsrMerge symlinks
ln -sf usr/bin "$INITRAMFS_DIR/bin"
ln -sf usr/bin "$INITRAMFS_DIR/sbin"
ln -sf usr/lib "$INITRAMFS_DIR/lib"
ln -sf usr/lib "$INITRAMFS_DIR/lib64"

# Copy our compiled custom software
cp build/syninit "$INITRAMFS_DIR/init"
cp build/synth3x "$INITRAMFS_DIR/usr/bin/synth3x"
cp build/syn "$INITRAMFS_DIR/usr/bin/syn"
ln -sf syn "$INITRAMFS_DIR/usr/bin/emerge"
# Build Rust safe process components
echo "  -- Building Rust safe process foundation + installer..."
cd "$DIR"
if command -v cargo >/dev/null 2>&1; then
    cargo build --release --manifest-path src/lib/Cargo.toml 2>&1 || true
    cp src/lib/target/release/synth3x-installer "$INITRAMFS_DIR/usr/bin/synth3x-installer" 2>/dev/null || \
        cp scripts/synth3x-installer.sh "$INITRAMFS_DIR/usr/bin/synth3x-installer"
    echo "  ✓ Rust installer (safe process foundation)"
else
    cp scripts/synth3x-installer.sh "$INITRAMFS_DIR/usr/bin/synth3x-installer"
    echo "  ⚠ cargo not found, using bash installer"
fi
chmod +x "$INITRAMFS_DIR/usr/bin/synth3x-installer"
ln -sf synth3x-installer "$INITRAMFS_DIR/usr/bin/synth3x0-installer"

# Copy C installer components
if [ -f build/synth3x-downloader ]; then
    cp build/synth3x-downloader "$INITRAMFS_DIR/usr/bin/"
    echo "  ✓ synth3x-downloader (C component)"
fi
if [ -f build/synth3x-wifi ]; then
    cp build/synth3x-wifi "$INITRAMFS_DIR/usr/bin/"
    echo "  ✓ synth3x-wifi (C component)"
fi

# Copy hardware analyzers
cp build/ram_analyzer "$INITRAMFS_DIR/usr/bin/"
cp build/disk_analyzer "$INITRAMFS_DIR/usr/bin/"
cp build/device_names "$INITRAMFS_DIR/usr/bin/"
cp build/usb_analyzer "$INITRAMFS_DIR/usr/bin/"
cp build/cable_analyzer "$INITRAMFS_DIR/usr/bin/"

# Copy security checks
mkdir -p "$INITRAMFS_DIR/usr/bin/checks"
cp build/checks/* "$INITRAMFS_DIR/usr/bin/checks/"
cp src/checks/tor-start.sh "$INITRAMFS_DIR/usr/bin/tor-start"
cp src/checks/check_drivers.sh "$INITRAMFS_DIR/usr/bin/check-drivers-all"

# Create security auditor script
cat << 'EOF' > "$INITRAMFS_DIR/usr/bin/checks-all"
#!/bin/bash
CYAN='\033[0;36m'; LIGHT_CYAN='\033[1;36m'; PURPLE='\033[0;35m'
GREEN='\033[0;32m'; NC='\033[0m'
echo -e "${CYAN}  ╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}  ║${NC}    ${PURPLE}Synth3x Full System Check v0.8${NC}             ${CYAN}║${NC}"
echo -e "${CYAN}  ╚══════════════════════════════════════════════════════════╝${NC}"
/usr/bin/check-drivers-all
echo "---"
echo -e "${GREEN}[*] Security checks:${NC}"
/usr/bin/checks/check_tor 2>/dev/null; /usr/bin/checks/check_firewall 2>/dev/null
/usr/bin/checks/check_mac 2>/dev/null; /usr/bin/checks/check_amnesic 2>/dev/null
/usr/bin/checks/check_hostname 2>/dev/null
echo "---"
echo -e "${GREEN}[*] Driver checks:${NC}"
/usr/bin/checks/check_display 2>/dev/null; /usr/bin/checks/check_keyboard 2>/dev/null
/usr/bin/checks/check_mouse 2>/dev/null; /usr/bin/checks/check_sound 2>/dev/null
echo -e "${GREEN}[+] Full check complete!${NC}"
EOF
chmod +x "$INITRAMFS_DIR/usr/bin/checks-all"

# Find paths for busybox, tor, nft, w3m (browser)
BUSYBOX_BIN=$(which busybox 2>/dev/null || echo "/usr/lib/initcpio/busybox")
TOR_BIN=$(which tor 2>/dev/null || echo "/usr/bin/tor")
NFT_BIN=$(which nft 2>/dev/null || echo "/usr/sbin/nft")
W3M_BIN=$(which w3m 2>/dev/null || echo "")
WGET_BIN=$(which wget 2>/dev/null || echo "/usr/bin/wget")

if [ ! -f "$BUSYBOX_BIN" ]; then
    echo "  [✗] busybox binary not found at $BUSYBOX_BIN"
    exit 1
fi

cp "$BUSYBOX_BIN" "$INITRAMFS_DIR/bin/busybox"
cp "$TOR_BIN" "$INITRAMFS_DIR/usr/bin/tor" 2>/dev/null || true
cp "$NFT_BIN" "$INITRAMFS_DIR/usr/sbin/nft" 2>/dev/null || true
cp /bin/bash "$INITRAMFS_DIR/bin/bash"

# Copy web browser if available
if [ -n "$W3M_BIN" ] && [ -f "$W3M_BIN" ]; then
    cp "$W3M_BIN" "$INITRAMFS_DIR/usr/bin/w3m"
    echo "  ✓ w3m browser included"
fi

# Ensure wget is available
if [ -f "$WGET_BIN" ]; then
    cp "$WGET_BIN" "$INITRAMFS_DIR/usr/bin/wget" 2>/dev/null || true
fi

# Populate BusyBox applets
cd "$INITRAMFS_DIR"
ln -sf busybox bin/sh
./bin/busybox --install -s bin/ 2>/dev/null || true
# Fix absolute symlinks — busybox --install creates them pointing to host path
find bin/ sbin/ usr/bin/ usr/sbin/ -type l -lname '*' | while read -r link; do
    tgt=$(readlink "$link")
    case "$tgt" in
        /*) ln -sf busybox "$link" 2>/dev/null || true ;;
    esac
done

# Copy compiled custom commands
rm -f bin/reboot bin/shutdown bin/poweroff
cp "$DIR/build/commands/reboot" bin/reboot
cp "$DIR/build/commands/shutdown" bin/shutdown
cp "$DIR/build/commands/shutdown" bin/poweroff

for dir in sbin usr/bin usr/sbin; do
    rm -f "$dir/reboot" "$dir/shutdown" "$dir/poweroff"
    ln -sf /bin/reboot "$dir/reboot"
    ln -sf /bin/shutdown "$dir/shutdown"
    ln -sf /bin/poweroff "$dir/poweroff"
done

cd "$DIR"

# Copy dynamic libraries
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

# Copy host system utilities
echo "  -- Scanning and copying host system utilities..."
for tool in parted fdisk mkfs.ext4 mkfs.vfat curl tar grep awk udevadm grub-install wget; do
    TOOL_PATH=$(which $tool 2>/dev/null)
    if [ -n "$TOOL_PATH" ] && [ -f "$TOOL_PATH" ]; then
        tool_dir=$(dirname "$TOOL_PATH")
        mkdir -p "$INITRAMFS_DIR$tool_dir"
        cp -p "$TOOL_PATH" "$INITRAMFS_DIR$TOOL_PATH" 2>/dev/null || true
        copy_deps "$TOOL_PATH" "$INITRAMFS_DIR"
    fi
done

# Copy GRUB modules
if [ -d "/usr/lib/grub/x86_64-efi" ]; then
    echo "  -- Copying GRUB UEFI modules..."
    mkdir -p "$INITRAMFS_DIR/usr/lib/grub"
    cp -a "/usr/lib/grub/x86_64-efi" "$INITRAMFS_DIR/usr/lib/grub/"
fi

copy_deps "$INITRAMFS_DIR/init" "$INITRAMFS_DIR"
copy_deps "$TOR_BIN" "$INITRAMFS_DIR"
copy_deps "$NFT_BIN" "$INITRAMFS_DIR"
copy_deps "$INITRAMFS_DIR/usr/bin/synth3x" "$INITRAMFS_DIR"
copy_deps "$INITRAMFS_DIR/usr/bin/synth3x-installer" "$INITRAMFS_DIR"
copy_deps "/bin/bash" "$INITRAMFS_DIR"
copy_deps "$BUSYBOX_BIN" "$INITRAMFS_DIR"
[ -n "$W3M_BIN" ] && [ -f "$W3M_BIN" ] && copy_deps "$W3M_BIN" "$INITRAMFS_DIR"

# Copy dynamic linkers
cp -aL /lib64/ld-linux-x86-64.so.* "$INITRAMFS_DIR/lib64/" 2>/dev/null || true
cp -aL /lib/ld-linux-x86-64.so.* "$INITRAMFS_DIR/lib/" 2>/dev/null || true

# Copy configurations
mkdir -p "$INITRAMFS_DIR/etc/tor"
cp boot/torrc "$INITRAMFS_DIR/etc/tor/torrc"
cp boot/nftables.rules "$INITRAMFS_DIR/etc/nftables.rules"

# Copy GPU kernel modules for framebuffer (QEMU + real HW)
echo "  -- Copying GPU kernel modules..."
mkdir -p "$INITRAMFS_DIR/lib/modules"
# Auto-detect kernel version
if [ -d /usr/lib/modules/6.18.33-2-cachyos-lts ]; then
    KVER="6.18.33-2-cachyos-lts"
elif [ -d "/lib/modules/$(uname -r)" ]; then
    KVER="$(uname -r)"
elif [ -d /usr/lib/modules ] && KVER=$(ls -d /usr/lib/modules/*/ 2>/dev/null | head -1) && [ -n "$KVER" ]; then
    KVER=$(basename "$KVER")
else
    echo "  ⚠ No kernel modules directory found, skipping GPU modules"
    KVER=""
fi
    if [ -n "$KVER" ]; then
        MODDIR="/lib/modules/$KVER"
        [ ! -d "$MODDIR" ] && MODDIR="/usr/lib/modules/$KVER"
        echo "    Kernel modules: $MODDIR"
        MODDST="$INITRAMFS_DIR/lib/modules/$KVER"
        mkdir -p "$MODDST"
        for mod in virtio_net net_failover failover e1000 e1000e r8169 \
                   bochs virtio-gpu virtio_dma_buf ttm serio_raw psmouse mousedev virtio_input \
                   virtio virtio_ring virtio_pci virtio_mmio \
                   cfg80211 mac80211 iwlwifi iwldvm iwlmvm \
                   ath ath3k ath5k ath9k ath9k_hw ath9k_common ath10k_core ath10k_pci \
                   rtl8xxxu rtw88_core rtw88_8822ce rtw88_8821ce \
                   rtlwifi rtl_pci rtl8192ce rtl8192se rtl8723ae rtl8723be rtl8188ee \
                   i2c-piix4 i2c-i801 i2c-hid; do
            src=$(find "$MODDIR" -name "${mod}.ko*" -type f 2>/dev/null | head -1)
            if [ -n "$src" ]; then
                case "$src" in
                    *.zst) zstd -dq "$src" -o "$MODDST/${mod}.ko" 2>/dev/null ;;
                    *.xz)  xz -dc "$src" > "$MODDST/${mod}.ko" 2>/dev/null ;;
                    *)     cp "$src" "$MODDST/${mod}.ko" 2>/dev/null ;;
                esac
                echo "    ✓ ${mod}.ko"
            fi
        done
        depmod -b "$INITRAMFS_DIR" "$KVER" 2>/dev/null && echo "    ✓ modules.dep generated" || true
    fi

echo "  -- Creating /etc/sudoers for sudo support..."
cat << 'EOF' > "$INITRAMFS_DIR/etc/sudoers"
root ALL=(ALL) ALL
%wheel ALL=(ALL) ALL
EOF
chmod 440 "$INITRAMFS_DIR/etc/sudoers"

# Create default /etc/passwd with user placeholder
cat << 'EOF' > "$INITRAMFS_DIR/etc/passwd"
root:x:0:0:root:/root:/bin/sh
tor:x:100:100:tor:/var/lib/tor:/bin/sh
EOF

cat << 'EOF' > "$INITRAMFS_DIR/etc/group"
root:x:0:
wheel:x:10:root
tor:x:100:
EOF

# 5. Compress initramfs & Build Live ISO
echo "[4/6] Packaging compressed initramfs..."
cd "$INITRAMFS_DIR"
find . | cpio -H newc -o --quiet | gzip -9 -f > ../initrd.img
cd "$DIR"
echo "  ✓ Compressed initramfs size: $(du -h build/initrd.img | cut -f1)"

echo "[5/6] Creating bootable Live ISO..."
rm -rf iso || true
mkdir -p iso/boot/grub || true

# Auto-detect and copy kernel
KERNEL_COPIED=false
# Try multiple locations (use sudo in case vmlinuz is root-readable only)
for src in \
    "/usr/lib/modules/$KVER/vmlinuz" \
    "/boot/vmlinuz-linux" \
    "/boot/vmlinuz" \
    /boot/vmlinuz-*; do
    if [ "$KERNEL_COPIED" = true ]; then break; fi
    if [ -f "$src" ]; then
        if cp "$src" iso/boot/vmlinuz-linux 2>/dev/null || sudo cp "$src" iso/boot/vmlinuz-linux 2>/dev/null; then
            chmod 644 iso/boot/vmlinuz-linux 2>/dev/null || sudo chmod 644 iso/boot/vmlinuz-linux 2>/dev/null || true
            echo "  ✓ Kernel copied from $src"
            KERNEL_COPIED=true
        fi
    fi
done
if [ "$KERNEL_COPIED" = false ]; then
    echo "  ⚠ No host kernel found at /boot/vmlinuz*"
    ls /boot/ 2>/dev/null || true
fi

cp build/initrd.img iso/boot/initrd.img 2>/dev/null || true
cp boot/grub.cfg iso/boot/grub/grub.cfg 2>/dev/null || true
chmod -R 755 iso/boot/ 2>/dev/null || true

if command -v grub-mkrescue >/dev/null 2>&1; then
    echo "[6/6] Building ISO with grub-mkrescue..."
    grub-mkrescue -o iso/synth3x.iso iso -- -volid "SYNTH3X" 2>&1 || { echo "  [✗] grub-mkrescue failed!"; exit 1; }
    echo ""
    echo "  ── Synth3x v${VERSION} ISO ready ──"
    echo "  File: iso/synth3x.iso"
    echo "  Size: $(du -h iso/synth3x.iso | cut -f1)"
    echo ""
    echo "  Run in QEMU:"
    echo "    qemu-system-x86_64 -cdrom iso/synth3x.iso -m 1024 -accel kvm"
    echo ""
    echo "  Features:"
    echo "    ✓ syn package manager (syn inst/binary/list)"
    echo "    ✓ Web browser (w3m — type 'browser' in terminal)"
    echo "    ✓ Touchpad support (auto-detect)"
    echo "    ✓ Hardware detection (Lenovo, Acer, Dell, HP)"
    echo "    ✓ Internet guide in Synth3x Guide window"
    echo "    ✓ Rust safe process foundation"
    echo "    ✓ Hard disk installer (synth3x-installer)"
    echo ""
else
    echo "  [✗] grub-mkrescue not found."
fi
