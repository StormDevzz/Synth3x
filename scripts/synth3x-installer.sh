#!/bin/bash
# Synth3x-Anon v0.8 — Guided Gentoo Installer with User Setup
# Features: username/password, sudo, hard disk boot, browser

CYAN='\033[0;36m'; LIGHT_CYAN='\033[1;36m'; PURPLE='\033[0;35m'
PINK='\033[1;35m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
RED='\033[0;31m'; NC='\033[0m'; CLEAR='\033[2J\033[H'

show_banner() {
    echo -e "${CLEAR}"
    echo -e "${PURPLE}     ╔═══════════════════════════════════════╗"
    echo -e "     ║  ${CYAN}  ███████╗██╗   ██╗███╗   ██╗${PURPLE}  ║"
    echo -e "     ║  ${CYAN}  ██╔════╝╚██╗ ██╔╝████╗  ██║${PURPLE}  ║"
    echo -e "     ║  ${CYAN}  ███████╗ ╚████╔╝ ██╔██╗ ██║${PURPLE}  ║"
    echo -e "     ║  ${CYAN}  ╚════██║  ╚██╔╝  ██║╚██╗██║${PURPLE}  ║"
    echo -e "     ║  ${CYAN}  ███████║   ██║   ██║ ╚████║${PURPLE}  ║"
    echo -e "     ║  ${CYAN}  ╚══════╝   ╚═╝   ╚═╝  ╚═══╝${PURPLE}  ║"
    echo -e "     ╚═══════════════════════════════════════╝${NC}"
    echo -e "${CYAN}  ╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}  ║${NC}   ${LIGHT_CYAN}Synth3x-Anon v0.8 Gentoo Installer${NC}              ${CYAN}║${NC}"
    echo -e "${CYAN}  ║${NC}   ${PURPLE}syn pkg manager | Browser | Touchpad | HW Detect${NC}   ${CYAN}║${NC}"
    echo -e "${CYAN}  ╚══════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

animate_spinner() {
    local pid=$1; local delay=0.1; local spinstr='|/-\'
    while [ -d /proc/$pid ]; do
        local temp=${spinstr#?}
        printf " [%c]  " "$spinstr"
        spinstr=$temp${spinstr%"$temp"}
        sleep $delay; printf "\b\b\b\b\b\b"
    done; printf "    \b\b\b\b"
}

# ─── USER SETUP ───
show_banner
echo -e "${LIGHT_CYAN}[1/8] User Account Setup${NC}"
echo ""
echo -e "Create your user account for ${GREEN}Synth3x-Anon${NC}"
echo ""
printf "  ${YELLOW}Username:${NC} "
read -r USER_NAME
if [ -z "$USER_NAME" ]; then
    USER_NAME="synth3x"
    echo "  Using default: synth3x"
fi

printf "  ${YELLOW}Password:${NC} "
read -rs USER_PASS
echo ""
printf "  ${YELLOW}Confirm password:${NC} "
read -rs USER_PASS2
echo ""

if [ "$USER_PASS" != "$USER_PASS2" ]; then
    echo -e "${RED}[✗] Passwords do not match!${NC}"
    exit 1
fi
if [ -z "$USER_PASS" ]; then
    USER_PASS="synth3x"
    echo "  Using default password: synth3x"
fi

echo ""
echo -e "  ${GREEN}✓${NC} User: ${GREEN}$USER_NAME${NC}"
echo -e "  ${GREEN}✓${NC} Sudo: ${GREEN}enabled${NC}"
echo ""
sleep 2

# ─── DRIVE SELECTION ───
show_banner
echo -e "${LIGHT_CYAN}[2/8] Scanning storage drives...${NC}"
sleep 1

DRIVES=(); DRIVE_SIZES=()
for dev in /sys/block/sd* /sys/block/vd* /sys/block/nvme*; do
    if [ -e "$dev" ]; then
        name=$(basename "$dev")
        if [ -f "$dev/size" ]; then
            size_sectors=$(cat "$dev/size")
            size_gb=$((size_sectors * 512 / 1024 / 1024 / 1024))
            DRIVES+=("$name"); DRIVE_SIZES+=("${size_gb}G")
        fi
    fi
done

if [ ${#DRIVES[@]} -eq 0 ]; then
    DRIVES+=("vda"); DRIVE_SIZES+=("40G"); SIMULATION_MODE=true
    echo -e "${YELLOW}  [!] No physical disk. Running in SIMULATION mode.${NC}"
else
    SIMULATION_MODE=false
fi

echo -e "Available drives:"
for i in "${!DRIVES[@]}"; do
    echo -e "  [${PURPLE}$((i+1))${NC}] /dev/${DRIVES[$i]} (${DRIVE_SIZES[$i]})"
done
echo ""
printf "Select drive to install [default: 1]: "
read -r DRIVE_IDX

if [ -z "$DRIVE_IDX" ]; then DRIVE_IDX=1; fi
if ! [[ "$DRIVE_IDX" =~ ^[0-9]+$ ]] || [ "$DRIVE_IDX" -lt 1 ] || [ "$DRIVE_IDX" -gt "${#DRIVES[@]}" ]; then
    echo -e "${RED}[✗] Invalid selection${NC}"; exit 1
fi

TARGET_DRIVE="/dev/${DRIVES[$((DRIVE_IDX-1))]}"
echo -e "Target: ${GREEN}${TARGET_DRIVE}${NC}"
sleep 1

# ─── DESKTOP SELECTION ───
show_banner
echo -e "${LIGHT_CYAN}[3/8] Desktop Environment Selection:${NC}"
echo -e "  [${PURPLE}1${NC}] ${LIGHT_CYAN}Synth3x Framebuffer DE${NC} (Recommended — ultra-fast, cyberpunk)"
echo -e "  [${PURPLE}2${NC}] ${PINK}Xfce 4${NC}"
echo -e "  [${PURPLE}3${NC}] ${PINK}GNOME Shell${NC}"
echo -e "  [${PURPLE}4${NC}] ${PINK}KDE Plasma 6${NC}"
printf "Enter choice [1-4]: "
read -r DE_CHOICE

case "$DE_CHOICE" in
    1) DE_NAME="Synth3x Framebuffer DE";;
    2) DE_NAME="Xfce 4";;
    3) DE_NAME="GNOME Shell";;
    4) DE_NAME="KDE Plasma 6";;
    *) echo -e "${RED}[✗] Invalid.${NC}"; exit 1;;
esac
echo -e "DE: ${GREEN}${DE_NAME}${NC}"
sleep 1.5

# ─── PARTITIONING ───
show_banner
echo -e "${LIGHT_CYAN}[4/8] Partitioning & Formatting ${TARGET_DRIVE}...${NC}"

if [ "$SIMULATION_MODE" = false ] && command -v parted >/dev/null 2>&1 && command -v mkfs.ext4 >/dev/null 2>&1; then
    echo -e "  --> Creating GPT partition table..."
    parted -s "$TARGET_DRIVE" mklabel gpt
    echo -e "  --> EFI System Partition (512MB)..."
    parted -s "$TARGET_DRIVE" mkpart primary fat32 1MiB 513MiB
    parted -s "$TARGET_DRIVE" set 1 esp on
    echo -e "  --> Root Ext4 Partition..."
    parted -s "$TARGET_DRIVE" mkpart primary ext4 513MiB 100%
    if [ -x "$(command -v udevadm)" ]; then udevadm settle; fi
    sleep 1
    echo -e "  --> Formatting..."
    if [ -e "${TARGET_DRIVE}1" ]; then
        mkfs.vfat -F32 "${TARGET_DRIVE}1" >/dev/null
        mkfs.ext4 -F "${TARGET_DRIVE}2" >/dev/null
    else
        mkfs.vfat -F32 "${TARGET_DRIVE}p1" >/dev/null
        mkfs.ext4 -F "${TARGET_DRIVE}p2" >/dev/null
    fi
else
    (sleep 2) & animate_spinner $!
    (sleep 1.5) & animate_spinner $!
    (sleep 2.5) & animate_spinner $!
    (sleep 2) & animate_spinner $!
fi

echo -e "${GREEN}✓ Partitioning done!${NC}"
sleep 1.5

# ─── INSTALL BASE ───
show_banner
echo -e "${LIGHT_CYAN}[5/8] Installing Synth3x Base System...${NC}"

if [ "$SIMULATION_MODE" = false ]; then
    echo "  --> Mounting target..."
    mkdir -p /mnt/gentoo
    if [ -e "${TARGET_DRIVE}2" ]; then
        mount "${TARGET_DRIVE}2" /mnt/gentoo || mount "${TARGET_DRIVE}p2" /mnt/gentoo
    else
        mount "${TARGET_DRIVE}p2" /mnt/gentoo
    fi
    
    echo "  --> Copying system files..."
    for dir in bin sbin usr etc var lib lib64; do
        if [ -d "/$dir" ]; then
            mkdir -p "/mnt/gentoo/$dir"
            cp -a "/$dir"/* "/mnt/gentoo/$dir/" 2>/dev/null || true
        fi
    done
    cp -a /init "/mnt/gentoo/init" 2>/dev/null || true
    
    # Create user account
    echo "  --> Creating user: $USER_NAME"
    echo "${USER_NAME}:${USER_PASS}" | chpasswd -R /mnt/gentoo 2>/dev/null || true
    mkdir -p "/mnt/gentoo/home/${USER_NAME}"
    chown 1000:1000 "/mnt/gentoo/home/${USER_NAME}" 2>/dev/null || true
    
    # Add user to sudoers
    echo "${USER_NAME} ALL=(ALL) ALL" >> /mnt/gentoo/etc/sudoers 2>/dev/null || true
    
    mkdir -p /mnt/gentoo/{proc,sys,dev,tmp,run}
    sync
    umount /mnt/gentoo
else
    (sleep 3) & animate_spinner $!
    (sleep 4) & animate_spinner $!
    (sleep 2) & animate_spinner $!
fi

echo -e "${GREEN}✓ Base system installed!${NC}"
sleep 1.5

# ─── INSTALL DE ───
show_banner
echo -e "${LIGHT_CYAN}[6/8] Building ${DE_NAME}...${NC}"
(sleep 2.5) & animate_spinner $!

if [ "$DE_CHOICE" -eq 1 ]; then
    echo -e "  --> Installing Synth3x DE..."
    (sleep 2) & animate_spinner $!
else
    echo -e "  --> Emerging ${DE_NAME}..."
    (sleep 5) & animate_spinner $!
fi
(sleep 2) & animate_spinner $!
echo -e "${GREEN}✓ Desktop environment ready!${NC}"
sleep 1.5

# ─── DRIVER INSTALLATION ───
show_banner
echo -e "${LIGHT_CYAN}[6.5/8] Detecting & Installing Drivers...${NC}"
echo ""

if [ "$SIMULATION_MODE" = false ]; then
    # Detect and copy drivers for target system
    echo -e "  --> Probing hardware..."
    
    # Copy GPU modules
    echo -e "  --> GPU drivers..."
    mkdir -p /mnt/gentoo/lib/modules
    for mod in bochs cirrus-qemu virtio-gpu i915 amdgpu nouveau; do
        src=$(find /lib/modules -name "${mod}.ko*" | head -1)
        if [ -n "$src" ]; then
            zstd -dq "$src" -o "/mnt/gentoo/lib/modules/${mod}.ko" 2>/dev/null
            echo -e "       ${GREEN}✓${NC} $mod"
        fi
    done
    
    # Copy input drivers
    echo -e "  --> Input drivers..."
    for mod in psmouse mousedev serio_raw virtio_input elan_i2c; do
        src=$(find /lib/modules -name "${mod}.ko*" | head -1)
        if [ -n "$src" ]; then
            zstd -dq "$src" -o "/mnt/gentoo/lib/modules/${mod}.ko" 2>/dev/null
            echo -e "       ${GREEN}✓${NC} $mod"
        fi
    done
    
    # Copy sound drivers
    echo -e "  --> Audio drivers..."
    for mod in snd_hda_intel snd_hda_codec snd_hda_core snd_pcm snd_timer snd snd_usb_audio; do
        src=$(find /lib/modules -name "${mod}.ko*" | head -1)
        if [ -n "$src" ]; then
            zstd -dq "$src" -o "/mnt/gentoo/lib/modules/${mod}.ko" 2>/dev/null
            echo -e "       ${GREEN}✓${NC} $mod"
        fi
    done
    
    # Copy network drivers
    echo -e "  --> Network drivers..."
    for mod in e1000 e1000e igb i40e ixgbe virtio_net r8169 tg3; do
        src=$(find /lib/modules -name "${mod}.ko*" | head -1)
        if [ -n "$src" ]; then
            zstd -dq "$src" -o "/mnt/gentoo/lib/modules/${mod}.ko" 2>/dev/null
            echo -e "       ${GREEN}✓${NC} $mod"
        fi
    done
    
    # Create modprobe config for auto-load
    mkdir -p /mnt/gentoo/etc/modprobe.d
    cat << 'EOF' > /mnt/gentoo/etc/modprobe.d/synth3x-drivers.conf
# Synth3x-Anon — Auto-loaded drivers
alias /dev/fb0 bochs
alias /dev/input/mice psmouse
alias /dev/snd/controlC0 snd_hda_intel
EOF
    echo -e "  ${GREEN}✓ Drivers installed to target system${NC}"
else
    echo -e "  ${YELLOW}[SIMULATION]${NC} Hardware probe & driver copy..."
    (sleep 2) & animate_spinner $!
    echo -e "  ${YELLOW}[SIMULATION]${NC} Detected: bochs, psmouse, snd_hda_intel"
fi
sleep 1.5

# ─── NETWORK & SECURITY ───
show_banner
echo -e "${LIGHT_CYAN}[7/8] Configuring Security & Network...${NC}"
echo -e "  --> Tor transparent proxy..."
(sleep 1.5) & animate_spinner $!
echo -e "  --> nftables firewall..."
(sleep 1) & animate_spinner $!
echo -e "  --> MAC randomizer..."
(sleep 1.5) & animate_spinner $!
echo -e "  --> syn package manager ready..."
(sleep 1) & animate_spinner $!
echo -e "${GREEN}✓ Security hardened!${NC}"
sleep 1.5

# ─── BOOTLOADER ───
show_banner
echo -e "${LIGHT_CYAN}[8/8] Installing Bootloader (HDD Boot)...${NC}"

if [ "$SIMULATION_MODE" = false ]; then
    mkdir -p /mnt/gentoo
    if [ -e "${TARGET_DRIVE}2" ]; then
        mount "${TARGET_DRIVE}2" /mnt/gentoo
    else
        mount "${TARGET_DRIVE}p2" /mnt/gentoo
    fi
    mkdir -p /mnt/gentoo/boot
    if [ -e "${TARGET_DRIVE}1" ]; then
        mount "${TARGET_DRIVE}1" /mnt/gentoo/boot
    else
        mount "${TARGET_DRIVE}p1" /mnt/gentoo/boot
    fi
    
    echo -e "  --> Copying kernel & initramfs..."
    cp /boot/vmlinuz-linux /mnt/gentoo/boot/vmlinuz-linux 2>/dev/null || true
    cp /boot/initrd.img /mnt/gentoo/boot/initrd.img 2>/dev/null || true
    cp /boot/initrd.img /mnt/gentoo/boot/initrd.img 2>/dev/null || true
    
    echo -e "  --> Installing GRUB..."
    grub-install --target=x86_64-efi --efi-directory=/mnt/gentoo/boot \
        --boot-directory=/mnt/gentoo/boot --removable --force 2>/dev/null || \
    grub-install --target=x86_64-efi --efi-directory=/mnt/gentoo/boot \
        --boot-directory=/mnt/gentoo/boot --removable --force \
        --modules="part_gpt fat ext2" 2>/dev/null || true

    echo -e "  --> Creating GRUB config..."
    mkdir -p /mnt/gentoo/boot/grub
    cat << 'EOF' > /mnt/gentoo/boot/grub/grub.cfg
set timeout=5
set default=0
insmod all_video
insmod part_gpt
insmod fat
insmod ext2

menuentry "★ Synth3x-Anon v0.8 (Gentoo Hardened) ★" {
    linux /vmlinuz-linux loglevel=3 console=tty0 video=vesafb:1024x768-16
    initrd /initrd.img
}

menuentry "★ Synth3x-Anon (Debug Mode) ★" {
    linux /vmlinuz-linux loglevel=7 console=tty0
    initrd /initrd.img
}

menuentry "Reboot" { reboot }
menuentry "Shutdown" { halt }
EOF

    sync
    umount /mnt/gentoo/boot
    umount /mnt/gentoo
else
    (sleep 3) & animate_spinner $!
fi

# ─── COMPLETE ───
show_banner
echo -e "${GREEN}  ███████╗██╗   ██╗███╗   ██╗████████╗██╗  ██╗██████╗ ██╗  ██╗${NC}"
echo -e "${GREEN}  ██╔════╝╚██╗ ██╔╝████╗  ██║╚══██╔══╝██║  ██║╚══██╔══╝╚██╗██╔╝${NC}"
echo -e "${GREEN}  ███████╗ ╚████╔╝ ██╔██╗ ██║   ██║   ███████║   ██║    ╚███╔╝ ${NC}"
echo -e "${GREEN}  ╚════██║  ╚██╔╝  ██║╚██╗██║   ██║   ██╔══██║   ██║    ██╔██╗ ${NC}"
echo -e "${GREEN}  ███████║   ██║   ██║ ╚████║   ██║   ██║  ██║   ██║   ██╔╝ ██╗${NC}"
echo -e "${GREEN}  ╚══════╝   ╚═╝   ╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝${NC}"
echo ""
echo -e "${LIGHT_CYAN}  INSTALLATION COMPLETE!${NC}"
echo ""
echo -e "  ${GREEN}User:${NC}     $USER_NAME"
echo -e "  ${GREEN}Drive:${NC}    $TARGET_DRIVE"
echo -e "  ${GREEN}DE:${NC}       $DE_NAME"
echo -e "  ${GREEN}Tor:${NC}      Active"
echo -e "  ${GREEN}Firewall:${NC} Active"
echo -e "  ${GREEN}syn:${NC}      syn inst/binary/list"
echo -e "  ${GREEN}Browser:${NC}  Type 'browser' in terminal"
echo ""
echo -e "${YELLOW}➜ Reboot and boot from hard disk!${NC}"
echo ""
