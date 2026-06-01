#!/bin/bash
# Synth3x-Anon — Comprehensive Driver Health Check
# Checks: display, keyboard, mouse, sound, network, storage
CYAN='\033[0;36m'; GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; NC='\033[0m'

echo -e "${CYAN}"
echo "  ╔══════════════════════════════════════════════════════════╗"
echo "  ║     Synth3x-Anon Driver Health Check v0.8               ║"
echo "  ╚══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# --- Display ---
echo -e "  ${YELLOW}[Display]${NC}"
if [ -c /dev/fb0 ]; then
    fbset 2>/dev/null | head -3
    echo -e "  ${GREEN}✓${NC} Framebuffer active"
else
    echo -e "  ${RED}✗${NC} No /dev/fb0"
fi

# --- Keyboard ---
echo -e "\n  ${YELLOW}[Keyboard]${NC}"
if ls /dev/input/by-path/*kbd* 2>/dev/null | grep -q .; then
    echo -e "  ${GREEN}✓${NC} Keyboard detected"
elif ls /dev/input/by-path/*keyboard* 2>/dev/null | grep -q .; then
    echo -e "  ${GREEN}✓${NC} Keyboard detected"
elif grep -q "keyboard\|Keyboard" /proc/bus/input/devices 2>/dev/null; then
    echo -e "  ${GREEN}✓${NC} Keyboard in /proc/bus/input/devices"
else
    echo -e "  ${RED}✗${NC} No keyboard found"
fi

# --- Mouse / Touchpad ---
echo -e "\n  ${YELLOW}[Mouse/Touchpad]${NC}"
if [ -c /dev/input/mice ]; then
    echo -e "  ${GREEN}✓${NC} /dev/input/mice exists"
else
    echo -e "  ${RED}✗${NC} No /dev/input/mice"
fi
if grep -qi "touchpad\|synaptics\|elan" /proc/bus/input/devices 2>/dev/null; then
    echo -e "  ${GREEN}✓${NC} Touchpad detected"
elif grep -qi "mouse" /proc/bus/input/devices 2>/dev/null; then
    echo -e "  ${GREEN}✓${NC} Mouse detected"
else
    echo -e "  ${YELLOW}⚠${NC} No pointing device found"
fi

# --- Sound ---
echo -e "\n  ${YELLOW}[Sound]${NC}"
if [ -d /dev/snd ]; then
    echo -e "  ${GREEN}✓${NC} ALSA devices present"
    ls /dev/snd/ 2>/dev/null | head -5 | while read d; do echo "       /dev/snd/$d"; done
else
    echo -e "  ${RED}✗${NC} No ALSA devices"
fi

# --- Network ---
echo -e "\n  ${YELLOW}[Network]${NC}"
for iface in /sys/class/net/*; do
    name=$(basename "$iface")
    [ "$name" = "lo" ] && continue
    oper=$(cat "$iface/operstate" 2>/dev/null)
    echo "       $name: $oper"
done

# --- Storage ---
echo -e "\n  ${YELLOW}[Storage]${NC}"
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT 2>/dev/null | head -10 || echo "  No block devices"

# --- Modules ---
echo -e "\n  ${YELLOW}[Kernel Modules]${NC}"
for mod in bochs psmouse snd_hda_intel virtio_input; do
    if grep -q "^${mod}" /proc/modules 2>/dev/null; then
        echo -e "  ${GREEN}✓${NC} $mod loaded"
    else
        echo -e "  ${YELLOW}⚠${NC} $mod not loaded"
    fi
done

echo -e "\n  ${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "  Run individual checks: check_sound, check_keyboard,"
echo -e "  check_mouse, check_display, check_network"
echo -e "  ${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
