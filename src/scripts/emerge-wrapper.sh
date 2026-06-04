#!/bin/sh
# emerge wrapper — runs emerge from installed Gentoo system
MOUNT="/mnt/gentoo"
if [ -x "$MOUNT/usr/bin/emerge" ]; then
    # Mount required filesystems if not already mounted
    mount | grep -q "$MOUNT/proc"  || mount --bind /proc "$MOUNT/proc" 2>/dev/null
    mount | grep -q "$MOUNT/sys"   || mount --bind /sys  "$MOUNT/sys"  2>/dev/null
    mount | grep -q "$MOUNT/dev"   || mount --bind /dev  "$MOUNT/dev"  2>/dev/null
    # Chroot and run emerge
    exec chroot "$MOUNT" /usr/bin/emerge "$@"
else
    echo " [!] emerge is not available yet."
    echo "     Run 'synth3x-installer' to install Gentoo base system first."
    echo "     After installation, emerge will work in the installed system."
    echo ""
    echo "     HINT: You can also run: synth3x-help"
    exit 1
fi
