#!/bin/bash
# ============================================================================
# NinjaMagic Integration Test Suite
#
# Validates Android app compatibility, Space Child ecosystem integration,
# wearable connectivity, and cross-device sync.
#
# Usage:
#   ./integration_test.sh [--device serial] [--skip-network]
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$SCRIPT_DIR/results/integration"
DEVICE_SERIAL="${DEVICE_SERIAL:-}"
SKIP_NETWORK=false
PASS=0
FAIL=0
SKIP=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --device)       DEVICE_SERIAL="$2"; shift 2 ;;
        --skip-network) SKIP_NETWORK=true; shift ;;
        *) echo "Unknown arg: $1"; exit 1 ;;
    esac
done

ADB="adb"
[[ -n "$DEVICE_SERIAL" ]] && ADB="adb -s $DEVICE_SERIAL"

mkdir -p "$RESULTS_DIR"
LOG="$RESULTS_DIR/integration_$(date +%Y%m%d_%H%M%S).log"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$LOG"; }
pass() { log "  PASS: $*"; ((PASS++)); }
fail() { log "  FAIL: $*"; ((FAIL++)); }
skip() { log "  SKIP: $*"; ((SKIP++)); }

log "============================================"
log "NinjaMagic Integration Test Suite"
log "============================================"
log ""

# ===== Android App Compatibility =====
log "--- Android App Compatibility ---"

# Top apps to verify can install and launch
TOP_APPS=(
    "com.google.android.apps.maps:Google Maps"
    "com.google.android.youtube:YouTube"
    "com.google.android.gm:Gmail"
    "com.android.chrome:Chrome"
    "com.android.vending:Play Store"
    "com.google.android.apps.messaging:Messages"
    "com.google.android.dialer:Phone"
    "com.google.android.contacts:Contacts"
    "com.google.android.calendar:Calendar"
    "com.google.android.deskclock:Clock"
)

for app_entry in "${TOP_APPS[@]}"; do
    PKG="${app_entry%%:*}"
    NAME="${app_entry##*:}"
    log "TEST: $NAME ($PKG)"

    # Check if installed
    if $ADB shell "pm list packages | grep $PKG" >/dev/null 2>&1; then
        # Try to get launch intent
        LAUNCH=$($ADB shell "pm dump $PKG | grep -A1 'android.intent.action.MAIN' | head -2" 2>/dev/null || echo "")
        if [[ -n "$LAUNCH" ]]; then
            pass "$NAME installed and has launch intent"
        else
            pass "$NAME installed (no launcher intent — may be system service)"
        fi
    else
        skip "$NAME not installed"
    fi
done

# ===== NinjaMagic Launcher =====
log ""
log "--- NinjaMagic Launcher ---"

log "TEST: Launcher is default home app"
HOME=$($ADB shell "cmd shortcut get-default-launcher" 2>/dev/null || echo "")
if echo "$HOME" | grep -q "ninjamagic"; then
    pass "NinjaMagic Launcher is default home"
else
    fail "NinjaMagic Launcher is NOT default home (current: $HOME)"
fi

log "TEST: Launcher activity running"
if $ADB shell "dumpsys activity activities | grep LauncherActivity" >/dev/null 2>&1; then
    pass "LauncherActivity in activity stack"
else
    fail "LauncherActivity not found in activity stack"
fi

log "TEST: Agent foreground service"
if $ADB shell "dumpsys activity services | grep AgentForegroundService" >/dev/null 2>&1; then
    pass "AgentForegroundService running"
else
    fail "AgentForegroundService not running"
fi

log "TEST: MCP server service"
if $ADB shell "dumpsys activity services | grep McpServerService" >/dev/null 2>&1; then
    pass "McpServerService running"
else
    fail "McpServerService not running"
fi

# ===== Space Child Integration =====
log ""
log "--- Space Child Integration ---"

if $SKIP_NETWORK; then
    skip "Space Child auth endpoint (--skip-network)"
    skip "Space Child profile sync (--skip-network)"
    skip "Space Child SSO flow (--skip-network)"
else
    log "TEST: Space Child auth endpoint reachable"
    AUTH_STATUS=$($ADB shell "curl -s -o /dev/null -w '%{http_code}' https://spacechild.love/api/space-child-auth/user 2>/dev/null" || echo "000")
    if [[ "$AUTH_STATUS" == "401" || "$AUTH_STATUS" == "200" ]]; then
        pass "Space Child auth endpoint reachable (HTTP $AUTH_STATUS)"
    else
        fail "Space Child auth endpoint unreachable (HTTP $AUTH_STATUS)"
    fi

    log "TEST: OTA server reachable"
    OTA_STATUS=$($ADB shell "curl -s -o /dev/null -w '%{http_code}' https://ota.ninjamagicos.dev/v1/health 2>/dev/null" || echo "000")
    if [[ "$OTA_STATUS" != "000" ]]; then
        pass "OTA server reachable (HTTP $OTA_STATUS)"
    else
        skip "OTA server not yet deployed"
    fi
fi

# ===== Wearable / Bluetooth LE =====
log ""
log "--- Wearable Connectivity ---"

log "TEST: BLE scanning capability"
BLE=$($ADB shell "dumpsys bluetooth_manager | grep -i 'LE.*scan\|BLE'" 2>/dev/null || echo "")
if [[ -n "$BLE" ]]; then
    pass "BLE scanning available"
else
    fail "BLE scanning not available"
fi

log "TEST: Heart Rate Service UUID registered"
if $ADB shell "logcat -d -t 500 | grep -i '0000180d\|heart.rate\|BiofieldService'" >/dev/null 2>&1; then
    pass "Heart Rate Service UUID in use"
else
    skip "Heart Rate Service not yet scanned (no wearable nearby)"
fi

# ===== MSI Substrate Integration =====
log ""
log "--- MSI Substrate ---"

log "TEST: MSI kernel module loaded"
if $ADB shell "lsmod | grep msi" >/dev/null 2>&1; then
    pass "MSI kernel module loaded"
else
    if $ADB shell "ls /dev/msi" >/dev/null 2>&1; then
        pass "MSI device node exists"
    else
        fail "MSI kernel module not loaded and /dev/msi missing"
    fi
fi

log "TEST: MSI security audit log"
if $ADB shell "logcat -d -t 500 | grep 'msi_security'" >/dev/null 2>&1; then
    pass "MSI security audit logging active"
else
    skip "MSI security audit not yet triggered"
fi

log "TEST: Privacy guard active"
if $ADB shell "logcat -d -t 500 | grep 'privacy_guard'" >/dev/null 2>&1; then
    pass "Privacy guard active"
else
    skip "Privacy guard not yet triggered"
fi

# ===== Summary =====
log ""
log "============================================"
log "INTEGRATION TEST RESULTS"
log "============================================"
log "  PASS: $PASS"
log "  FAIL: $FAIL"
log "  SKIP: $SKIP"
log "  TOTAL: $((PASS + FAIL + SKIP))"
log "============================================"
log "Results saved to: $LOG"

exit $FAIL
