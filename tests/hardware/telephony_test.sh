#!/bin/bash
# ============================================================================
# NinjaMagic Hardware Test Suite — Telephony
#
# Validates full telephony stack on both target devices:
#   - Pixel 7 (panther) — Tensor GS201 / Samsung Shannon modem
#   - Nord N30 (larry)  — Snapdragon 695 / Qualcomm X53 modem
#
# Prerequisites:
#   - Device connected via USB with adb access
#   - SIM card inserted with active service
#   - ninjamagicOS flashed and booted
#
# Usage:
#   ./telephony_test.sh [--device serial] [--skip-calls] [--test-number +1234567890]
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$SCRIPT_DIR/results/telephony"
DEVICE_SERIAL="${DEVICE_SERIAL:-}"
SKIP_CALLS=false
TEST_NUMBER=""
PASS=0
FAIL=0
SKIP=0

# Parse args
while [[ $# -gt 0 ]]; do
    case $1 in
        --device)    DEVICE_SERIAL="$2"; shift 2 ;;
        --skip-calls) SKIP_CALLS=true; shift ;;
        --test-number) TEST_NUMBER="$2"; shift 2 ;;
        *) echo "Unknown arg: $1"; exit 1 ;;
    esac
done

ADB="adb"
[[ -n "$DEVICE_SERIAL" ]] && ADB="adb -s $DEVICE_SERIAL"

mkdir -p "$RESULTS_DIR"
LOG="$RESULTS_DIR/telephony_$(date +%Y%m%d_%H%M%S).log"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$LOG"; }
pass() { log "  PASS: $*"; ((PASS++)); }
fail() { log "  FAIL: $*"; ((FAIL++)); }
skip() { log "  SKIP: $*"; ((SKIP++)); }

log "============================================"
log "NinjaMagic Telephony Test Suite"
log "============================================"
log "Device: $($ADB shell getprop ro.product.model 2>/dev/null || echo 'unknown')"
log "Build:  $($ADB shell getprop ro.build.display.id 2>/dev/null || echo 'unknown')"
log ""

# ===== Test 1: RILD Service =====
log "TEST 1: NinjaMagic RILD Service"
if $ADB shell "ps -A | grep ninjamagic-rild" >/dev/null 2>&1; then
    pass "ninjamagic-rild process running"
else
    fail "ninjamagic-rild process not found"
fi

# ===== Test 2: MSI Bridge =====
log "TEST 2: MSI Event Bridge"
if $ADB shell "logcat -d -t 100 | grep 'MSI bridge initialized'" >/dev/null 2>&1; then
    pass "MSI bridge initialized"
else
    fail "MSI bridge not initialized"
fi

# ===== Test 3: SIM Detection =====
log "TEST 3: SIM Card Detection"
SIM_STATE=$($ADB shell "getprop gsm.sim.state" 2>/dev/null || echo "UNKNOWN")
if [[ "$SIM_STATE" == "READY" ]]; then
    pass "SIM state: READY"
else
    fail "SIM state: $SIM_STATE (expected READY)"
fi

# ===== Test 4: Network Registration =====
log "TEST 4: Network Registration"
NET_TYPE=$($ADB shell "getprop gsm.network.type" 2>/dev/null || echo "UNKNOWN")
if [[ "$NET_TYPE" != "" && "$NET_TYPE" != "UNKNOWN" ]]; then
    pass "Network type: $NET_TYPE"
else
    fail "Network type: $NET_TYPE (not registered)"
fi

# ===== Test 5: Signal Strength =====
log "TEST 5: Signal Strength"
SIGNAL=$($ADB shell "dumpsys telephony.registry | grep -i signalstrength | head -1" 2>/dev/null || echo "")
if [[ -n "$SIGNAL" ]]; then
    pass "Signal strength reported: $(echo $SIGNAL | head -c 80)"
else
    fail "No signal strength data"
fi

# ===== Test 6: Data Connectivity =====
log "TEST 6: Mobile Data Connectivity"
DATA_STATE=$($ADB shell "getprop gsm.nitz.time" 2>/dev/null || echo "")
PING=$($ADB shell "ping -c 1 -W 5 8.8.8.8 2>/dev/null" || echo "")
if echo "$PING" | grep -q "1 received"; then
    pass "Data connectivity: ping successful"
else
    fail "Data connectivity: ping failed"
fi

# ===== Test 7: VoLTE Registration =====
log "TEST 7: VoLTE Registration"
IMS=$($ADB shell "dumpsys telephony.registry | grep -i ims" 2>/dev/null || echo "")
if echo "$IMS" | grep -qi "registered"; then
    pass "IMS/VoLTE registered"
else
    skip "IMS/VoLTE not detected (may not be supported by carrier)"
fi

# ===== Test 8: Outgoing Call =====
log "TEST 8: Outgoing Call"
if $SKIP_CALLS || [[ -z "$TEST_NUMBER" ]]; then
    skip "Outgoing call (--skip-calls or no --test-number)"
else
    log "  Dialing $TEST_NUMBER..."
    $ADB shell "am start -a android.intent.action.CALL -d tel:$TEST_NUMBER" >/dev/null 2>&1
    sleep 5
    CALL_STATE=$($ADB shell "dumpsys telephony.registry | grep mCallState" 2>/dev/null || echo "")
    if echo "$CALL_STATE" | grep -q "1\|2"; then
        pass "Outgoing call initiated"
        sleep 2
        $ADB shell "input keyevent KEYCODE_ENDCALL" >/dev/null 2>&1
    else
        fail "Outgoing call failed to initiate"
    fi
fi

# ===== Test 9: SMS Send =====
log "TEST 9: SMS Send"
if $SKIP_CALLS || [[ -z "$TEST_NUMBER" ]]; then
    skip "SMS send (--skip-calls or no --test-number)"
else
    log "  Sending test SMS to $TEST_NUMBER..."
    $ADB shell "service call isms 7 i32 0 s16 \"$TEST_NUMBER\" s16 \"null\" s16 \"ninjamagicOS test\" s16 \"null\" s16 \"null\"" >/dev/null 2>&1
    sleep 3
    if $ADB shell "logcat -d -t 20 | grep -i 'sms.*sent\|sms.*deliver'" >/dev/null 2>&1; then
        pass "SMS sent successfully"
    else
        fail "SMS send not confirmed"
    fi
fi

# ===== Test 10: Emergency Number Detection =====
log "TEST 10: Emergency Number Detection"
EMERGENCY=$($ADB shell "dumpsys phone | grep -i emergency" 2>/dev/null || echo "")
if [[ -n "$EMERGENCY" ]]; then
    pass "Emergency number detection present"
else
    fail "Emergency number detection not found"
fi

# ===== Test 11: MSI Telephony Events =====
log "TEST 11: MSI Telephony Event Bus"
MSI_EVENTS=$($ADB shell "logcat -d -t 200 | grep 'msi.*telephony\|rild.*event'" 2>/dev/null || echo "")
if [[ -n "$MSI_EVENTS" ]]; then
    pass "MSI telephony events flowing"
else
    fail "No MSI telephony events detected"
fi

# ===== Test 12: Airplane Mode Toggle =====
log "TEST 12: Airplane Mode Toggle"
$ADB shell "settings put global airplane_mode_on 1" >/dev/null 2>&1
$ADB shell "am broadcast -a android.intent.action.AIRPLANE_MODE" >/dev/null 2>&1
sleep 3
AP_ON=$($ADB shell "settings get global airplane_mode_on" 2>/dev/null || echo "")
$ADB shell "settings put global airplane_mode_on 0" >/dev/null 2>&1
$ADB shell "am broadcast -a android.intent.action.AIRPLANE_MODE" >/dev/null 2>&1
sleep 5
AP_OFF=$($ADB shell "settings get global airplane_mode_on" 2>/dev/null || echo "")
if [[ "$AP_ON" == "1" && "$AP_OFF" == "0" ]]; then
    pass "Airplane mode toggle works"
else
    fail "Airplane mode toggle failed (on=$AP_ON off=$AP_OFF)"
fi

# ===== Summary =====
log ""
log "============================================"
log "TELEPHONY TEST RESULTS"
log "============================================"
log "  PASS: $PASS"
log "  FAIL: $FAIL"
log "  SKIP: $SKIP"
log "  TOTAL: $((PASS + FAIL + SKIP))"
log "============================================"
log "Results saved to: $LOG"

exit $FAIL
