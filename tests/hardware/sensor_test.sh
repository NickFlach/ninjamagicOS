#!/bin/bash
# ============================================================================
# NinjaMagic Hardware Test Suite — Sensors
#
# Validates all device sensors on Pixel 7 and Nord N30.
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$SCRIPT_DIR/results/sensors"
DEVICE_SERIAL="${DEVICE_SERIAL:-}"
PASS=0
FAIL=0
SKIP=0

ADB="adb"
[[ -n "$DEVICE_SERIAL" ]] && ADB="adb -s $DEVICE_SERIAL"

mkdir -p "$RESULTS_DIR"
LOG="$RESULTS_DIR/sensor_$(date +%Y%m%d_%H%M%S).log"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$LOG"; }
pass() { log "  PASS: $*"; ((PASS++)); }
fail() { log "  FAIL: $*"; ((FAIL++)); }
skip() { log "  SKIP: $*"; ((SKIP++)); }

log "============================================"
log "NinjaMagic Sensor Test Suite"
log "============================================"
log "Device: $($ADB shell getprop ro.product.model 2>/dev/null || echo 'unknown')"
log ""

# Get sensor list
SENSOR_LIST=$($ADB shell "dumpsys sensorservice | grep -E '^\w'" 2>/dev/null || echo "")

check_sensor() {
    local name="$1"
    local pattern="$2"
    log "TEST: $name"
    if echo "$SENSOR_LIST" | grep -qi "$pattern"; then
        pass "$name detected"
    else
        fail "$name NOT detected"
    fi
}

check_sensor "Accelerometer"        "accelerometer\|accel"
check_sensor "Gyroscope"            "gyroscope\|gyro"
check_sensor "Magnetometer"         "magnetic\|magnetometer"
check_sensor "Barometer"            "pressure\|barometer"
check_sensor "Proximity"            "proximity"
check_sensor "Ambient Light"        "light"
check_sensor "Gravity"              "gravity"
check_sensor "Linear Acceleration"  "linear"
check_sensor "Rotation Vector"      "rotation"
check_sensor "Step Counter"         "step.*counter"

# GPS test
log "TEST: GPS/Location"
GPS=$($ADB shell "dumpsys location | grep -i 'gps\|gnss'" 2>/dev/null || echo "")
if [[ -n "$GPS" ]]; then
    pass "GPS/GNSS service present"
else
    fail "GPS/GNSS service not found"
fi

# Bluetooth
log "TEST: Bluetooth"
BT=$($ADB shell "getprop bluetooth.status" 2>/dev/null || echo "")
BT_ADDR=$($ADB shell "settings get secure bluetooth_address" 2>/dev/null || echo "")
if [[ -n "$BT_ADDR" && "$BT_ADDR" != "null" ]]; then
    pass "Bluetooth available (addr: $BT_ADDR)"
else
    fail "Bluetooth not detected"
fi

# WiFi
log "TEST: WiFi Hardware"
WIFI=$($ADB shell "dumpsys wifi | grep 'Wi-Fi is'" 2>/dev/null || echo "")
if echo "$WIFI" | grep -qi "enabled\|disabled"; then
    pass "WiFi hardware present"
else
    fail "WiFi hardware not found"
fi

# NFC (Pixel 7 only)
log "TEST: NFC"
NFC=$($ADB shell "dumpsys nfc | grep -i 'state'" 2>/dev/null || echo "")
if [[ -n "$NFC" ]]; then
    pass "NFC present"
else
    skip "NFC not found (may not be available on this device)"
fi

# Fingerprint
log "TEST: Fingerprint Sensor"
FP=$($ADB shell "dumpsys fingerprint | grep -i 'sensor'" 2>/dev/null || echo "")
if [[ -n "$FP" ]]; then
    pass "Fingerprint sensor present"
else
    fail "Fingerprint sensor not found"
fi

# Camera sensors
log "TEST: Camera Sensors"
CAM_COUNT=$($ADB shell "dumpsys media.camera | grep -c 'Camera Id'" 2>/dev/null || echo "0")
if [[ "$CAM_COUNT" -gt 0 ]]; then
    pass "Camera sensors: $CAM_COUNT detected"
else
    fail "No camera sensors detected"
fi

# Battery/charging
log "TEST: Battery Monitoring"
BATTERY=$($ADB shell "dumpsys battery" 2>/dev/null || echo "")
if echo "$BATTERY" | grep -q "level:"; then
    LEVEL=$(echo "$BATTERY" | grep "level:" | awk '{print $2}')
    TEMP=$(echo "$BATTERY" | grep "temperature:" | awk '{print $2}')
    pass "Battery: ${LEVEL}%, temp: ${TEMP}"
else
    fail "Battery monitoring not available"
fi

# Summary
log ""
log "============================================"
log "SENSOR TEST RESULTS"
log "============================================"
log "  PASS: $PASS"
log "  FAIL: $FAIL"
log "  SKIP: $SKIP"
log "  TOTAL: $((PASS + FAIL + SKIP))"
log "============================================"

exit $FAIL
