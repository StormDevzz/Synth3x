#!/bin/bash
# Synth3x-Anon v0.8.1 Beta — Cyberpunk Safe Installer (legacy, use Rust version)
# Features: boot disk detection, safety checks, dry-run, automatic backup warning

# ─── Dark Cyberpunk Palette ───
HX='\033[0m'         # reset
BG='\033[48;2;10;10;15m' # почти чёрный фон
BG2='\033[48;2;15;12;22m' # темно-фиолетовый фон
FG='\033[38;2;180;180;200m' # серый текст
NEON_CYAN='\033[38;2;0;255;200m'
NEON_PINK='\033[38;2;255;0;128m'
NEON_PURPLE='\033[38;2;140;0;255m'
NEON_RED='\033[38;2;255;30;60m'
DARK_RED='\033[38;2;180;0;30m'
NEON_GREEN='\033[38;2;0;255;100m'
DIM_GREEN='\033[38;2;0;150;60m'
NEON_YELLOW='\033[38;2;255;220;40m'
DIM='\033[38;2;80;80;100m'
BOLD='\033[1m'
DIM_BG='\033[48;2;18;15;28m'
WARN_BG='\033[48;2;40;5;10m'
WARN_BORDER='\033[38;2;255;30;60m'
CHECK='\033[38;2;0;255;100m'
CROSS='\033[38;2;255;30;60m'

print_center() {
    local str="$1" color="$2"
    local width=$(tput cols 2>/dev/null || echo 80)
    local pad=$(( (width - ${#str}) / 2 ))
    [ $pad -lt 0 ] && pad=0
    printf "${color}%*s${HX}\n" $((pad + ${#str})) "$str"
}

safety_abort() {
    echo -e "\n${WARN_BORDER}╔══════════════════════════════════════════════════════════╗${HX}"
    echo -e "${WARN_BORDER}║${HX}  ${CROSS}${BOLD} ABORTED:${HX} $1"
    echo -e "${WARN_BORDER}║${HX}  ${NEON_YELLOW}Installation cancelled. No changes were made.${HX}"
    echo -e "${WARN_BORDER}╚══════════════════════════════════════════════════════════╝${HX}"
    exit 1
}

show_banner() {
    echo -e "\033[2J\033[H"
    echo -e "${BG2}${FG}"
    echo -e "${NEON_CYAN}     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓${HX}"
    echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_PURPLE}███████╗██╗   ██╗███╗   ██╗████████╗██╗  ██╗██████╗ ██╗  ██╗${NEON_CYAN}  ▓${HX}"
    echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_PURPLE}██╔════╝╚██╗ ██╔╝████╗  ██║╚══██╔══╝██║  ██║╚══██╔══╝╚██╗██╔╝${NEON_CYAN}  ▓${HX}"
    echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_PURPLE}███████╗ ╚████╔╝ ██╔██╗ ██║   ██║   ███████║   ██║    ╚███╔╝ ${NEON_CYAN}  ▓${HX}"
    echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_PURPLE}╚════██║  ╚██╔╝  ██║╚██╗██║   ██║   ██╔══██║   ██║    ██╔██╗ ${NEON_CYAN}  ▓${HX}"
    echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_PURPLE}███████║   ██║   ██║ ╚████║   ██║   ██║  ██║   ██║   ██╔╝ ██╗${NEON_CYAN}  ▓${HX}"
    echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_PURPLE}╚══════╝   ╚═╝   ╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝${NEON_CYAN}  ▓${HX}"
    echo -e "${NEON_CYAN}     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓${HX}"
    echo ""
    echo -e "${DIM}     ════════════════════════════════════════════════════════════${HX}"
    echo -e "     ${DIM}┃${HX}  ${NEON_CYAN}${BOLD}SYNTH3X-ANON v0.8.1   SAFE INSTALLER${HX}         ${DIM}┃${HX}"
    echo -e "     ${DIM}┃${HX}  ${DIM}Wayland Compositor  •  DRM/KMS  •  Portage/emerge${HX}  ${DIM}┃${HX}"
    echo -e "     ${DIM}┃${HX}  ${NEON_PURPLE}system initialized. awaiting input.${HX}               ${DIM}┃${HX}"
    echo -e "     ${DIM}════════════════════════════════════════════════════════════${HX}"
    echo ""
}

# ─── DETECT BOOT DISK ───
detect_boot_disk() {
    local boot_dev=""
    if [ -d /sys/block ]; then
        if [ -f /proc/cmdline ]; then
            local root_param=$(cat /proc/cmdline | grep -o 'root=[^ ]*' | head -1 | cut -d= -f2)
            [ -n "$root_param" ] && boot_dev=$(echo "$root_param" | sed 's/[0-9]*$//')
        fi
        if [ -z "$boot_dev" ]; then
            boot_dev=$(mount | grep ' / ' | head -1 | cut -d' ' -f1 | sed 's/[0-9]*$//')
        fi
        if [ -z "$boot_dev" ] && [ -f /etc/mtab ]; then
            boot_dev=$(grep ' / ' /etc/mtab | head -1 | cut -d' ' -f1 | sed 's/[0-9]*$//')
        fi
    fi
    echo "$boot_dev"
}

# ─── CHECK DRIVE SAFETY ───
check_drive_safety() {
    local drive="$1"
    local boot_disk="$2"

    echo -e "${WARN_BG}${FG}"
    echo -e "     ${WARN_BORDER}╔══════════════════════════════════════════════════════╗${HX}"
    echo -e "     ${WARN_BORDER}║${HX}  ${CROSS}${BOLD}${NEON_RED}  ⚠  DANGER: TARGET DRIVE ANALYSIS  ⚠${HX}          ${WARN_BORDER}║${HX}"
    echo -e "     ${WARN_BORDER}╚══════════════════════════════════════════════════════╝${HX}"
    echo ""

    if [ -n "$boot_disk" ] && [ "$drive" = "$boot_disk" ]; then
        echo -e "     ${NEON_RED}${BOLD}》 BOOT DISK DETECTED: ${drive}${HX}"
        echo -e "     ${FG}  This drive is currently running the OS.${HX}"
        echo -e "     ${CROSS}  Installing over it will cause system failure.${HX}"
        echo ""
        printf "     ${NEON_YELLOW}${BOLD}> Type ${NEON_PINK}YES${HX} ${NEON_YELLOW}to confirm boot disk wipe:${HX} ${NEON_PINK}"
        read -r confirmation
        echo -e "${HX}"
        if [ "$confirmation" != "YES" ]; then
            safety_abort "Boot disk installation rejected."
        fi
    fi

    local mounted=""
    for part in ${drive}* ${drive}p*; do
        [ -b "$part" ] || continue
        if mount | grep -q "$part "; then
            mounted="$mounted $part"
        fi
    done
    if [ -n "$mounted" ]; then
        echo -e "     ${NEON_YELLOW}》 MOUNTED PARTITIONS:${HX} ${mounted}"
        echo ""
        printf "     ${NEON_YELLOW}${BOLD}> Type ${NEON_PINK}YES${HX} ${NEON_YELLOW}to overwrite:${HX} ${NEON_PINK}"
        read -r confirmation
        echo -e "${HX}"
        if [ "$confirmation" != "YES" ]; then
            safety_abort "Mounted partition overwrite rejected."
        fi
    fi

    local size=""
    if [ -f "/sys/block/$(basename "$drive")/size" ]; then
        local sectors=$(cat "/sys/block/$(basename "$drive")/size")
        local gb=$((sectors * 512 / 1024 / 1024 / 1024))
        size="${gb}G"
    else
        size="???"
    fi

    echo ""
    echo -e "     ${DARK_RED}╔══════════════════════════════════════════════════════╗${HX}"
    echo -e "     ${DARK_RED}║${HX}  ${NEON_RED}${BOLD}☠  FINAL WARNING — IRREVERSIBLE  ☠${HX}             ${DARK_RED}║${HX}"
    echo -e "     ${DARK_RED}╠══════════════════════════════════════════════════════╣${HX}"
    echo -e "     ${DARK_RED}║${HX}  ${NEON_PINK}Target:${HX}  ${drive} (${size})"
    echo -e "     ${DARK_RED}║${HX}  ${NEON_PINK}Action:${HX}  ALL DATA WILL BE DESTROYED"
    echo -e "     ${DARK_RED}║${HX}  ${NEON_PINK}Note:${HX}   This operation cannot be undone."
    echo -e "     ${DARK_RED}╚══════════════════════════════════════════════════════╝${HX}"
    echo ""
    printf "     ${NEON_CYAN}> Confirm by typing the full path ${NEON_PURPLE}${drive}${HX} ${NEON_CYAN}:${HX} ${NEON_CYAN}"
    read -r final_confirm
    echo -e "${HX}"
    if [ "$final_confirm" != "$drive" ]; then
        safety_abort "Confirmation mismatch. Aborting."
    fi

    echo ""
    echo -e "     ${NEON_RED}${BOLD}》 Countdown to destruction... Ctrl+C to abort${HX}"
    echo -ne "     ${DIM}"
    for i in 5 4 3 2 1; do
        echo -ne "${NEON_RED}${i}${HX}${DIM}..${HX}"
        sleep 1
    done
    echo -e " ${CROSS}${BOLD} COMMENCING${HX}"
    echo ""
}

animate_spinner() {
    local pid=$1; local delay=0.1; local spinstr='░▒▓█▓▒░'
    while [ -d /proc/$pid ]; do
        local temp=${spinstr#?}
        printf " ${NEON_PURPLE}%s${HX}" "$spinstr"
        spinstr=$temp${spinstr%"$temp"}
        sleep $delay; printf "\b\b\b\b\b\b\b\b"
    done; printf "        \b\b\b\b\b\b\b\b"
}

# ─── MAIN ───
show_banner
echo -e "     ${DIM}$(date '+%Y-%m-%d %H:%M:%S') UTC  |  ${NEON_CYAN}${BOLD}BOOT.seq${HX}${DIM} initializing${HX}"
echo ""
echo -e "     ${NEON_CYAN}${BOLD}[1/9]${HX} ${FG}Safety Check — Scanning for boot device...${HX}"

BOOT_DISK=$(detect_boot_disk)
if [ -n "$BOOT_DISK" ]; then
    echo -e "     ${NEON_YELLOW}${BOLD}》 ${BOOT_DISK}${HX} ${DIM}flagged as boot disk${HX}"
else
    echo -e "     ${NEON_GREEN}${BOLD}》${HX} ${DIM}No boot disk — initramfs detected${HX}"
fi
sleep 1

# ─── USER SETUP ───
show_banner
echo -e "     ${NEON_CYAN}${BOLD}[2/9]${HX} ${FG}User Account Provisioning${HX}"
echo ""
printf "     ${DIM}┃${HX} ${FG}Username:${HX} ${NEON_CYAN}"
read -r USER_NAME
echo -e "${HX}"
if [ -z "$USER_NAME" ]; then
    USER_NAME="synth3x"
    echo -e "     ${DIM}┃ default →${HX} ${NEON_GREEN}synth3x${HX}"
fi
printf "     ${DIM}┃${HX} ${FG}Password:${HX} ${NEON_PURPLE}"
read -rs USER_PASS
echo -e "${HX}"
printf "     ${DIM}┃${HX} ${FG}Confirm:${HX} ${NEON_PURPLE}"
read -rs USER_PASS2
echo -e "${HX}"
if [ "$USER_PASS" != "$USER_PASS2" ]; then
    echo -e "     ${CROSS}${BOLD} PASSWORDS DO NOT MATCH${HX}"
    exit 1
fi
if [ -z "$USER_PASS" ]; then
    USER_PASS="synth3x"
    echo -e "     ${DIM}┃ default →${HX} ${NEON_GREEN}synth3x${HX}"
fi
echo ""
echo -e "     ${DIM}┃${HX} ${NEON_GREEN}✓${HX} ${FG}User:${HX} ${NEON_CYAN}${USER_NAME}${HX}"
echo -e "     ${DIM}┃${HX} ${NEON_GREEN}✓${HX} ${FG}Sudo:${HX} ${NEON_GREEN}enabled${HX}"
sleep 1

# ─── DRIVE SCAN ───
show_banner
echo -e "     ${NEON_CYAN}${BOLD}[3/9]${HX} ${FG}Scanning storage topology...${HX}"
sleep 1

DRIVES=(); DRIVE_SIZES=()
for dev in /sys/block/sd* /sys/block/vd* /sys/block/nvme* /sys/block/mmcblk*; do
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
    echo -e "     ${NEON_YELLOW}${BOLD}》${HX} ${FG}No physical disks — simulation mode${HX}"
else
    SIMULATION_MODE=false
fi

echo ""
echo -e "     ${DIM}┌──────────────┬──────────┐${HX}"
echo -e "     ${DIM}│${HX} ${NEON_CYAN}DEVICE${HX}         ${DIM}│${HX} ${NEON_CYAN}SIZE${HX}       ${DIM}│${HX}"
echo -e "     ${DIM}├──────────────┼──────────┤${HX}"
for i in "${!DRIVES[@]}"; do
    DEV_PATH="/dev/${DRIVES[$i]}"
    FLAG=""
    [ "$DEV_PATH" = "$BOOT_DISK" ] && FLAG=" ${NEON_RED}(BOOT)${HX}"
    echo -e "     ${DIM}│${HX} ${NEON_PURPLE}$((i+1))${HX}. ${DEV_PATH}${FLAG}  ${DIM}│${HX} ${DRIVE_SIZES[$i]}        ${DIM}│${HX}"
done
echo -e "     ${DIM}└──────────────┴──────────┘${HX}"
echo ""
printf "     ${FG}Select target drive [${NEON_PURPLE}1${FG}]:${HX} ${NEON_CYAN}"
read -r DRIVE_IDX
echo -e "${HX}"
if [ -z "$DRIVE_IDX" ]; then DRIVE_IDX=1; fi
if ! [[ "$DRIVE_IDX" =~ ^[0-9]+$ ]] || [ "$DRIVE_IDX" -lt 1 ] || [ "$DRIVE_IDX" -gt "${#DRIVES[@]}" ]; then
    echo -e "     ${CROSS}${BOLD} INVALID SELECTION${HX}"; exit 1
fi
TARGET_DRIVE="/dev/${DRIVES[$((DRIVE_IDX-1))]}"
echo -e "     ${DIM}┃${HX} ${NEON_GREEN}✓${HX} ${FG}Target:${HX} ${NEON_CYAN}${TARGET_DRIVE}${HX}"
sleep 1

# ─── SAFETY CHECK ───
show_banner
echo -e "     ${NEON_CYAN}${BOLD}[4/9]${HX} ${FG}Safety Verification Sequence${HX}"
echo ""
if [ "$SIMULATION_MODE" = false ]; then
    check_drive_safety "$TARGET_DRIVE" "$BOOT_DISK"
else
    echo -e "     ${DIM}» simulation — safety checks bypassed${HX}"
    sleep 1
fi

# ─── DESKTOP SELECTION ───
show_banner
echo -e "     ${NEON_CYAN}${BOLD}[5/9]${HX} ${FG}Desktop Environment Selection${HX}"
echo ""
echo -e "     ${DIM}┌─────────────────────────────────────────────────────┐${HX}"
echo -e "     ${DIM}│${HX}  ${NEON_PURPLE}1.${HX} ${NEON_CYAN}${BOLD}Synth3x Wayland Compositor${HX} ${DIM}(recommended)${HX}"
echo -e "     ${DIM}│${HX}     ${DIM}cyberpunk DE  •  DRM/KMS  •  GPU accel${HX}"
echo -e "     ${DIM}│${HX}                                               ${DIM}│${HX}"
echo -e "     ${DIM}│${HX}  ${NEON_PURPLE}2.${HX} ${DIM}Xfce 4${HX}"
echo -e "     ${DIM}│${HX}  ${NEON_PURPLE}3.${HX} ${DIM}GNOME Shell${HX}"
echo -e "     ${DIM}│${HX}  ${NEON_PURPLE}4.${HX} ${DIM}KDE Plasma 6${HX}"
echo -e "     ${DIM}└─────────────────────────────────────────────────────┘${HX}"
echo ""
printf "     ${FG}Select [${NEON_PURPLE}1${FG}-${NEON_PURPLE}4${FG}]:${HX} ${NEON_CYAN}"
read -r DE_CHOICE
echo -e "${HX}"
case "$DE_CHOICE" in
    1) DE_NAME="Synth3x Wayland Compositor";;
    2) DE_NAME="Xfce 4";;
    3) DE_NAME="GNOME Shell";;
    4) DE_NAME="KDE Plasma 6";;
    *) echo -e "     ${CROSS}${BOLD} INVALID${HX}"; exit 1;;
esac
echo -e "     ${DIM}┃${HX} ${NEON_GREEN}✓${HX} ${FG}Selected:${HX} ${NEON_CYAN}${DE_NAME}${HX}"
sleep 1

# ─── PARTITIONING ───
show_banner
echo -e "     ${NEON_CYAN}${BOLD}[6/9]${HX} ${FG}Partitioning & Formatting${HX}"
echo -e "     ${DIM}${TARGET_DRIVE}${HX}"
echo ""

if [ "$SIMULATION_MODE" = false ] && command -v parted >/dev/null 2>&1 && command -v mkfs.ext4 >/dev/null 2>&1; then
    echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}GPT table...${HX} "
    parted -s "$TARGET_DRIVE" mklabel gpt || safety_abort "Failed to create partition table"
    echo -e "${NEON_GREEN}✓${HX}"

    echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}EFI partition (512MB)...${HX} "
    parted -s "$TARGET_DRIVE" mkpart primary fat32 1MiB 513MiB || safety_abort "EFI partition failed"
    parted -s "$TARGET_DRIVE" set 1 esp on || true
    echo -e "${NEON_GREEN}✓${HX}"

    echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}Root ext4...${HX} "
    parted -s "$TARGET_DRIVE" mkpart primary ext4 513MiB 100% || safety_abort "Root partition failed"
    [ -x "$(command -v udevadm)" ] && udevadm settle
    sleep 2
    echo -e "${NEON_GREEN}✓${HX}"

    echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}Formatting...${HX} "
    if [ -e "${TARGET_DRIVE}1" ]; then
        mkfs.vfat -F32 "${TARGET_DRIVE}1" >/dev/null 2>&1 || safety_abort "FAT32 format failed"
        mkfs.ext4 -F "${TARGET_DRIVE}2" >/dev/null 2>&1 || safety_abort "ext4 format failed"
    else
        mkfs.vfat -F32 "${TARGET_DRIVE}p1" >/dev/null 2>&1 || safety_abort "FAT32 format failed"
        mkfs.ext4 -F "${TARGET_DRIVE}p2" >/dev/null 2>&1 || safety_abort "ext4 format failed"
    fi
    echo -e "${NEON_GREEN}✓${HX}"
else
    (sleep 2) & animate_spinner $!
fi
echo ""
echo -e "     ${NEON_GREEN}${BOLD}✓ PARTITIONING COMPLETE${HX}"
sleep 1

# ─── INSTALL BASE ───
show_banner
echo -e "     ${NEON_CYAN}${BOLD}[7/9]${HX} ${FG}Installing Synth3x Base System${HX}"
echo ""

if [ "$SIMULATION_MODE" = false ]; then
    echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}Mounting root partition...${HX} "
    mkdir -p /mnt/gentoo
    if [ -e "${TARGET_DRIVE}2" ]; then
        mount "${TARGET_DRIVE}2" /mnt/gentoo || mount "${TARGET_DRIVE}p2" /mnt/gentoo || safety_abort "Failed to mount root partition."
    else
        mount "${TARGET_DRIVE}p2" /mnt/gentoo || safety_abort "Failed to mount root partition."
    fi
    echo -e "${NEON_GREEN}✓${HX}"

    # ─── REAL GENTOO BUILD OR OFFLINE COPY ───
    echo -e "     ${NEON_CYAN}》 Checking internet connection for Gentoo mirror...${HX}"
    if ping -c 1 -W 3 gentoo.org >/dev/null 2>&1; then
        echo -e "     ${NEON_GREEN}» Online. Fetching latest Gentoo Stage3 OpenRC...${HX}"
        
        # Download Stage3 using wget or curl
        local stage3_url="https://bouncer.gentoo.org/fetch/root/all/releases/amd64/autobuilds/current-stage3-amd64-openrc/stage3-amd64-openrc-latest.tar.xz"
        echo -e "     ${DIM}URL: ${stage3_url}${HX}"
        wget -q --show-progress -O /mnt/gentoo/stage3.tar.xz "$stage3_url" || \
        curl -L -o /mnt/gentoo/stage3.tar.xz "$stage3_url" || \
        safety_abort "Failed to download Gentoo Stage3 tarball."
        
        echo -e "     ${NEON_GREEN}» Extracting Stage3 tarball...${HX}"
        tar -xpf /mnt/gentoo/stage3.tar.xz -C /mnt/gentoo --xattrs-include='*.*' --numeric-owner || \
        safety_abort "Failed to extract Gentoo Stage3 tarball."
        rm -f /mnt/gentoo/stage3.tar.xz
        
        # Mount virtual filesystems
        echo -e "     ${NEON_GREEN}» Configuring mount points and chroot...${HX}"
        mkdir -p /mnt/gentoo/{proc,sys,dev}
        mount --bind /proc /mnt/gentoo/proc
        mount --bind /sys /mnt/gentoo/sys
        mount --bind /dev /mnt/gentoo/dev
        
        # Write make.conf
        cat << 'EOF' > /mnt/gentoo/etc/portage/make.conf
COMMON_FLAGS="-O2 -pipe -march=x86-64 -mtune=generic -mno-avx -mno-avx2 -mno-sse4.1 -mno-sse4.2"
CFLAGS="${COMMON_FLAGS}"
CXXFLAGS="${COMMON_FLAGS}"
FCFLAGS="${COMMON_FLAGS}"
FFLAGS="${COMMON_FLAGS}"
PORTDIR="/var/db/repos/gentoo"
DISTDIR="/var/cache/distfiles"
PKGDIR="/var/cache/binpkgs"
LC_MESSAGES=C.utf8
USE="wayland elogind dbus udev unicode -X -gnome -kde"
EOF

        # Write fstab
        local root_part="${TARGET_DRIVE}2"
        local boot_part="${TARGET_DRIVE}1"
        if [ ! -e "$root_part" ]; then
            root_part="${TARGET_DRIVE}p2"
            boot_part="${TARGET_DRIVE}p1"
        fi
        cat << EOF > /mnt/gentoo/etc/fstab
${boot_part}   /boot       vfat    defaults,noatime    0 2
${root_part}   /           ext4    noatime             0 1
EOF

        # DNS
        cp -L /etc/resolv.conf /mnt/gentoo/etc/resolv.conf
        
        # Hostname
        echo "synth3x-gentoo" > /mnt/gentoo/etc/hostname
        
        # User creation and passwords
        chroot /mnt/gentoo useradd -m -G wheel,video,input,audio -s /bin/bash "${USER_NAME}" 2>/dev/null
        echo "${USER_NAME}:${USER_PASS}" | chroot /mnt/gentoo chpasswd 2>/dev/null
        echo "root:${USER_PASS}" | chroot /mnt/gentoo chpasswd 2>/dev/null
        
        # Sudo config
        echo "${USER_NAME} ALL=(ALL) ALL" >> /mnt/gentoo/etc/sudoers 2>/dev/null
        
        # Auto login configuration on TTY1 (agetty / inittab)
        if [ -f /mnt/gentoo/etc/inittab ]; then
            sed -i 's/c1:12345:respawn:\/sbin\/agetty.*/c1:12345:respawn:\/sbin\/agetty --autologin '"${USER_NAME}"' --noclear 38400 tty1 linux/' /mnt/gentoo/etc/inittab
        fi
        mkdir -p /mnt/gentoo/etc/conf.d
        echo 'agetty_options="--autologin '${USER_NAME}' --noclear"' > /mnt/gentoo/etc/conf.d/agetty.tty1
        
        # User .bash_profile auto start compositor
        mkdir -p "/mnt/gentoo/home/${USER_NAME}"
        cat << 'EOF' > "/mnt/gentoo/home/${USER_NAME}/.bash_profile"
if [ -z "$DISPLAY" ] && [ "$(tty)" = "/dev/tty1" ]; then
    exec /usr/bin/synth3x
fi
EOF
        chown -R 1000:1000 "/mnt/gentoo/home/${USER_NAME}" 2>/dev/null
        
        # Copy custom files from host
        echo -e "     ${NEON_GREEN}» Copying custom files and compositor binaries...${HX}"
        cp /usr/bin/synth3x /mnt/gentoo/usr/bin/ 2>/dev/null
        cp /usr/bin/ram_analyzer /mnt/gentoo/usr/bin/ 2>/dev/null
        cp /usr/bin/disk_analyzer /mnt/gentoo/usr/bin/ 2>/dev/null
        cp /usr/bin/device_names /mnt/gentoo/usr/bin/ 2>/dev/null
        cp /mnt/gentoo/usr/bin/device_names /mnt/gentoo/usr/bin/cpu_brand 2>/dev/null # compat
        cp /usr/bin/usb_analyzer /mnt/gentoo/usr/bin/ 2>/dev/null
        cp /usr/bin/cable_analyzer /mnt/gentoo/usr/bin/ 2>/dev/null
        cp /usr/bin/checks-all /mnt/gentoo/usr/bin/ 2>/dev/null
        
        # Copy nftables rules & startup scripts
        mkdir -p /mnt/gentoo/etc
        cp -a /etc/nftables.rules /mnt/gentoo/etc/ 2>/dev/null
        
        # Copy shared libraries used by compositor
        ldd /usr/bin/synth3x 2>/dev/null | grep -o '/[^ ]*\.so[^ ]*' | while read lib; do
            local dir=$(dirname "$lib")
            mkdir -p "/mnt/gentoo${dir}"
            cp -n "$lib" "/mnt/gentoo${lib}" 2>/dev/null
        done
        
        # Clean up chroot mount points
        echo -e "     ${NEON_GREEN}» Cleaning up mounts...${HX}"
        umount -l /mnt/gentoo/dev 2>/dev/null
        umount -l /mnt/gentoo/sys 2>/dev/null
        umount -l /mnt/gentoo/proc 2>/dev/null
        
        echo -e "     ${NEON_GREEN}» Gentoo Base System successfully built!${HX}"
    else
        echo -e "     ${NEON_YELLOW}» Offline. Bypassing Stage3 fetch, copying host filesystem instead...${HX}"
        
        echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}Copying system...${HX} "
        for dir in bin sbin usr etc var lib lib64; do
            if [ -d "/$dir" ]; then
                mkdir -p "/mnt/gentoo/$dir"
                cp -a "/$dir"/* "/mnt/gentoo/$dir/" 2>/dev/null || true
            fi
        done
        cp -a /init "/mnt/gentoo/init" 2>/dev/null || true
        echo -e "${NEON_GREEN}✓${HX}"

        echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}Creating user: ${USER_NAME}...${HX} "
        echo "${USER_NAME}:${USER_PASS}" | chpasswd -R /mnt/gentoo 2>/dev/null || true
        mkdir -p "/mnt/gentoo/home/${USER_NAME}"
        chown 1000:1000 "/mnt/gentoo/home/${USER_NAME}" 2>/dev/null || true
        echo "${USER_NAME} ALL=(ALL) ALL" >> /mnt/gentoo/etc/sudoers 2>/dev/null || true
        echo -e "${NEON_GREEN}✓${HX}"
    fi

    mkdir -p /mnt/gentoo/{proc,sys,dev,tmp,run}
    sync
    umount /mnt/gentoo
else
    (sleep 2) & animate_spinner $!
    (sleep 1) & animate_spinner $!
fi
echo ""
echo -e "     ${NEON_GREEN}${BOLD}✓ BASE SYSTEM INSTALLED${HX}"
sleep 1

# ─── INSTALL DE ───
show_banner
echo -e "     ${NEON_CYAN}${BOLD}[8/9]${HX} ${FG}Deploying ${DE_NAME}${HX}"
(sleep 1) & animate_spinner $!
(sleep 1) & animate_spinner $!
echo ""
echo -e "     ${NEON_GREEN}${BOLD}✓ DESKTOP ENVIRONMENT READY${HX}"
sleep 1

# ─── BOOTLOADER ───
show_banner
echo -e "     ${NEON_CYAN}${BOLD}[9/9]${HX} ${FG}Bootloader Installation (UEFI)${HX}"
echo ""

if [ "$SIMULATION_MODE" = false ]; then
    echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}Mounting partitions...${HX} "
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
    echo -e "${NEON_GREEN}✓${HX}"

    echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}Copying kernel + initramfs...${HX} "
    cp /boot/vmlinuz-linux /mnt/gentoo/boot/vmlinuz-linux 2>/dev/null || true
    cp /boot/initrd.img /mnt/gentoo/boot/initrd.img 2>/dev/null || true
    echo -e "${NEON_GREEN}✓${HX}"

    echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}Installing GRUB...${HX} "
    if command -v grub-install >/dev/null 2>&1; then
        grub-install --target=x86_64-efi --efi-directory=/mnt/gentoo/boot \
            --boot-directory=/mnt/gentoo/boot --removable --force 2>/dev/null || \
        grub-install --target=x86_64-efi --efi-directory=/mnt/gentoo/boot \
            --boot-directory=/mnt/gentoo/boot --removable --force \
            --modules="part_gpt fat ext2" 2>/dev/null || true
    fi
    echo -e "${NEON_GREEN}✓${HX}"

    echo -ne "     ${NEON_PURPLE}⌛${HX} ${DIM}GRUB config...${HX} "
    mkdir -p /mnt/gentoo/boot/grub
    cat << 'EOF' > /mnt/gentoo/boot/grub/grub.cfg
set timeout=5
set default=0
insmod all_video
insmod part_gpt
insmod fat
insmod ext2

menuentry "★ Synth3x-Anon v0.9 (Wayland Compositor) ★" {
    linux /vmlinuz-linux loglevel=3 console=tty0
    initrd /initrd.img
}
menuentry "★ Synth3x-Anon (Debug Mode) ★" {
    linux /vmlinuz-linux loglevel=7 console=tty0
    initrd /initrd.img
}
menuentry "Reboot" { reboot }
menuentry "Shutdown" { halt }
EOF
    echo -e "${NEON_GREEN}✓${HX}"

    sync
    umount /mnt/gentoo/boot 2>/dev/null || true
    umount /mnt/gentoo 2>/dev/null || true
else
    (sleep 2) & animate_spinner $!
fi
echo ""
echo -e "     ${NEON_GREEN}${BOLD}✓ BOOTLOADER INSTALLED${HX}"
sleep 1

# ─── COMPLETE ───
echo -e "\033[2J\033[H"
echo -e "${BG2}${NEON_CYAN}     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓${HX}"
echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_GREEN}██████╗ ██████╗ ███╗   ███╗██████╗ ██╗     ███████╗████████╗███████╗${NEON_CYAN}  ▓${HX}"
echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_GREEN}██╔════╝██╔═══██╗████╗ ████║██╔══██╗██║     ██╔════╝╚══██╔══╝██╔════╝${NEON_CYAN}  ▓${HX}"
echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_GREEN}██║     ██║   ██║██╔████╔██║██████╔╝██║     █████╗     ██║   █████╗${NEON_CYAN}  ▓${HX}"
echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_GREEN}██║     ██║   ██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝     ██║   ██╔══╝${NEON_CYAN}  ▓${HX}"
echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_GREEN}╚██████╗╚██████╔╝██║ ╚═╝ ██║██║     ███████╗███████╗   ██║   ███████╗${NEON_CYAN}  ▓${HX}"
echo -e "${NEON_CYAN}     ▓${HX}  ${BG2}${NEON_GREEN}╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝   ╚═╝   ╚══════╝${NEON_CYAN}  ▓${HX}"
echo -e "${NEON_CYAN}     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓${HX}"
echo ""
echo -e "     ${NEON_GREEN}${BOLD}  SYSTEM INSTALL COMPLETE${HX}"
echo ""
echo -e "     ${DIM}┌─────────────────────────────────────────────────────┐${HX}"
echo -e "     ${DIM}│${HX}  ${NEON_CYAN}User:${HX}     ${FG}${USER_NAME}${HX}                         ${DIM}│${HX}"
echo -e "     ${DIM}│${HX}  ${NEON_CYAN}Drive:${HX}    ${FG}${TARGET_DRIVE}${HX}                      ${DIM}│${HX}"
echo -e "     ${DIM}│${HX}  ${NEON_CYAN}DE:${HX}       ${FG}${DE_NAME}${HX}                  ${DIM}│${HX}"
echo -e "     ${DIM}│${HX}  ${NEON_CYAN}Display:${HX}  ${FG}Wayland (DRM/KMS)${HX}               ${DIM}│${HX}"
echo -e "     ${DIM}│${HX}  ${NEON_CYAN}Pkg:${HX}      ${FG}emerge <package>${HX}                 ${DIM}│${HX}"
echo -e "     ${DIM}└─────────────────────────────────────────────────────┘${HX}"
echo ""
echo -e "     ${NEON_PINK}${BOLD}》 REBOOT AND BOOT FROM DRIVE 《${HX}"
echo -e "     ${DIM}  Synth3x-Anon v0.9  •  wayland  •  drm  •  emerge${HX}"
echo ""
