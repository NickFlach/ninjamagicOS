#!/bin/bash
# ============================================================================
# NinjaMagic Flash Tool
#
# Flashes ninjamagicOS to a connected device via fastboot.
# Supports Pixel 7 (panther) and OnePlus Nord N30 (larry).
#
# Usage:
#   ./flash.sh [--device panther|larry] [--slot a|b] [--wipe] [--unlock]
#
# Prerequisites:
#   - fastboot in PATH
#   - Device in fastboot mode (adb reboot bootloader)
#   - Images built in out/target/product/<device>/
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
DEVICE=""
SLOT="a"
WIPE=false
UNLOCK=false
SERIAL=""

# NinjaMagic branding
echo ""
echo "  ╔══════════════════════════════════════╗"
echo "  ║      NinjaMagic Flash Tool v0.1.0    ║"
echo "  ║          ninjamagicOS Installer       ║"
echo "  ╚══════════════════════════════════════╝"
echo ""

# Parse args
while [[ $# -gt 0 ]]; do
    case $1 in
        --device)  DEVICE="$2"; shift 2 ;;
        --slot)    SLOT="$2"; shift 2 ;;
        --wipe)    WIPE=true; shift ;;
        --unlock)  UNLOCK=true; shift ;;
        --serial)  SERIAL="$2"; shift 2 ;;
        --help)
            echo "Usage: ./flash.sh [--device panther|larry] [--slot a|b] [--wipe] [--unlock]"
            echo ""
            echo "Options:"
            echo "  --device   Target device (panther=Pixel 7, larry=Nord N30)"
            echo "  --slot     Target slot (a or b, default: a)"
            echo "  --wipe     Wipe userdata (factory reset)"
            echo "  --unlock   Unlock bootloader first (requires user confirmation on device)"
            echo "  --serial   Fastboot device serial"
            exit 0
            ;;
        *) echo "Unknown arg: $1"; exit 1 ;;
    esac
done

FASTBOOT="fastboot"
[[ -n "$SERIAL" ]] && FASTBOOT="fastboot -s $SERIAL"

# Auto-detect device if not specified
if [[ -z "$DEVICE" ]]; then
    echo "[*] Auto-detecting device..."
    PRODUCT=$($FASTBOOT getvar product 2>&1 | grep "product:" | awk '{print $2}' || echo "")
    case "$PRODUCT" in
        panther*) DEVICE="panther" ;;
        larry*|CPH2513*) DEVICE="larry" ;;
        *)
            echo "[!] Cannot auto-detect device. Use --device panther|larry"
            exit 1
            ;;
    esac
fi

echo "[*] Target device: $DEVICE"
echo "[*] Target slot: $SLOT"
echo "[*] Wipe userdata: $WIPE"
echo ""

# Set image directory
IMG_DIR="$ROOT_DIR/out/target/product/$DEVICE"
if [[ ! -d "$IMG_DIR" ]]; then
    echo "[!] Image directory not found: $IMG_DIR"
    echo "[!] Build ninjamagicOS first: lunch ninjamagic_${DEVICE}-userdebug && make -j\$(nproc)"
    exit 1
fi

# Verify required images exist
REQUIRED_IMAGES=(
    "boot.img"
    "vendor_boot.img"
    "system.img"
    "vendor.img"
    "vbmeta.img"
    "dtbo.img"
)

echo "[*] Checking images in $IMG_DIR..."
for img in "${REQUIRED_IMAGES[@]}"; do
    if [[ ! -f "$IMG_DIR/$img" ]]; then
        echo "[!] Missing required image: $img"
        exit 1
    fi
    SIZE=$(du -h "$IMG_DIR/$img" | awk '{print $1}')
    echo "  [ok] $img ($SIZE)"
done
echo ""

# Unlock bootloader if requested
if $UNLOCK; then
    echo "[*] Unlocking bootloader..."
    echo "[!] WARNING: This will FACTORY RESET the device!"
    echo "[!] Confirm unlock on the device screen."
    $FASTBOOT flashing unlock || true
    echo "[*] Waiting for device to reboot to fastboot..."
    sleep 10
fi

# Check bootloader lock state
LOCK_STATE=$($FASTBOOT getvar unlocked 2>&1 | grep "unlocked:" | awk '{print $2}' || echo "unknown")
if [[ "$LOCK_STATE" != "yes" ]]; then
    echo "[!] Bootloader is LOCKED. Use --unlock or unlock manually."
    echo "[!] Current state: $LOCK_STATE"
    exit 1
fi

echo "[*] Bootloader unlocked. Beginning flash..."
echo ""

# Flash vbmeta first (with verification disabled for dev builds)
echo "[*] Flashing vbmeta_${SLOT}..."
$FASTBOOT flash "vbmeta_${SLOT}" "$IMG_DIR/vbmeta.img" \
    --disable-verity --disable-verification

# Flash boot partition
echo "[*] Flashing boot_${SLOT}..."
$FASTBOOT flash "boot_${SLOT}" "$IMG_DIR/boot.img"

# Flash vendor_boot
echo "[*] Flashing vendor_boot_${SLOT}..."
$FASTBOOT flash "vendor_boot_${SLOT}" "$IMG_DIR/vendor_boot.img"

# Flash dtbo
echo "[*] Flashing dtbo_${SLOT}..."
$FASTBOOT flash "dtbo_${SLOT}" "$IMG_DIR/dtbo.img"

# Flash system (large, may take a while)
echo "[*] Flashing system_${SLOT} (this may take a few minutes)..."
$FASTBOOT flash "system_${SLOT}" "$IMG_DIR/system.img"

# Flash vendor
echo "[*] Flashing vendor_${SLOT}..."
$FASTBOOT flash "vendor_${SLOT}" "$IMG_DIR/vendor.img"

# Wipe userdata if requested
if $WIPE; then
    echo "[*] Wiping userdata..."
    $FASTBOOT erase userdata
    $FASTBOOT erase metadata
fi

# Set active slot
echo "[*] Setting active slot to ${SLOT}..."
$FASTBOOT set_active "${SLOT}"

echo ""
echo "  ╔══════════════════════════════════════╗"
echo "  ║         Flash Complete!              ║"
echo "  ╚══════════════════════════════════════╝"
echo ""
echo "[*] Device: $DEVICE (slot $SLOT)"
echo "[*] Rebooting into ninjamagicOS..."
echo ""

$FASTBOOT reboot

echo "[*] Device is rebooting. First boot may take 2-3 minutes."
echo "[*] The NinjaMagic first-run wizard will guide you through setup."
echo ""
echo "Welcome to ninjamagicOS."
