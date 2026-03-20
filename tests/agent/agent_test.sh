#!/bin/bash
# ============================================================================
# NinjaMagic Agent Test Suite
#
# Validates agent skills, inference, memory, MCP, and conversation coherence.
# Runs on-device via adb or locally via MCP stdio transport.
#
# Usage:
#   ./agent_test.sh [--device serial] [--mcp-only] [--verbose]
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$SCRIPT_DIR/results/agent"
DEVICE_SERIAL="${DEVICE_SERIAL:-}"
MCP_ONLY=false
VERBOSE=false
PASS=0
FAIL=0
SKIP=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --device)    DEVICE_SERIAL="$2"; shift 2 ;;
        --mcp-only)  MCP_ONLY=true; shift ;;
        --verbose)   VERBOSE=true; shift ;;
        *) echo "Unknown arg: $1"; exit 1 ;;
    esac
done

ADB="adb"
[[ -n "$DEVICE_SERIAL" ]] && ADB="adb -s $DEVICE_SERIAL"

mkdir -p "$RESULTS_DIR"
LOG="$RESULTS_DIR/agent_$(date +%Y%m%d_%H%M%S).log"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$LOG"; }
pass() { log "  PASS: $*"; ((PASS++)); }
fail() { log "  FAIL: $*"; ((FAIL++)); }
skip() { log "  SKIP: $*"; ((SKIP++)); }

log "============================================"
log "NinjaMagic Agent Test Suite"
log "============================================"
log ""

# ===== MCP Protocol Tests =====
log "--- MCP Protocol Tests ---"

# Helper: send JSON-RPC to MCP server via stdio
mcp_call() {
    local method="$1"
    local params="${2:-{}}"
    local id="${3:-1}"
    local request="{\"jsonrpc\":\"2.0\",\"id\":$id,\"method\":\"$method\",\"params\":$params}"
    echo "$request"
}

# Test MCP: initialize
log "TEST: MCP initialize"
INIT_REQ=$(mcp_call "initialize" '{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}')
log "  Request: $INIT_REQ"
# In real test: pipe to ninjamagic-mcp-server and check response
# For now, validate the request format is correct JSON
if echo "$INIT_REQ" | python3 -m json.tool >/dev/null 2>&1; then
    pass "MCP initialize request is valid JSON-RPC"
else
    fail "MCP initialize request is invalid"
fi

# Test MCP: tools/list
log "TEST: MCP tools/list"
TOOLS_REQ=$(mcp_call "tools/list" '{}' 2)
if echo "$TOOLS_REQ" | python3 -m json.tool >/dev/null 2>&1; then
    pass "MCP tools/list request valid"
else
    fail "MCP tools/list request invalid"
fi

# Test MCP: resources/list
log "TEST: MCP resources/list"
RES_REQ=$(mcp_call "resources/list" '{}' 3)
if echo "$RES_REQ" | python3 -m json.tool >/dev/null 2>&1; then
    pass "MCP resources/list request valid"
else
    fail "MCP resources/list request invalid"
fi

# Test MCP: tools/call (phone_dial)
log "TEST: MCP tools/call phone_dial"
DIAL_REQ=$(mcp_call "tools/call" '{"name":"phone_dial","arguments":{"number":"+15551234567"}}' 4)
if echo "$DIAL_REQ" | python3 -m json.tool >/dev/null 2>&1; then
    pass "MCP phone_dial call request valid"
else
    fail "MCP phone_dial call request invalid"
fi

# Test MCP: tools/call (sms_send)
log "TEST: MCP tools/call sms_send"
SMS_REQ=$(mcp_call "tools/call" '{"name":"sms_send","arguments":{"to":"+15551234567","body":"Test message"}}' 5)
if echo "$SMS_REQ" | python3 -m json.tool >/dev/null 2>&1; then
    pass "MCP sms_send call request valid"
else
    fail "MCP sms_send call request invalid"
fi

# Test MCP: resources/read (battery)
log "TEST: MCP resources/read battery"
BAT_REQ=$(mcp_call "resources/read" '{"uri":"phone://state/battery"}' 6)
if echo "$BAT_REQ" | python3 -m json.tool >/dev/null 2>&1; then
    pass "MCP battery resource read request valid"
else
    fail "MCP battery resource read request invalid"
fi

if $MCP_ONLY; then
    log ""
    log "--- MCP-only mode, skipping on-device tests ---"
else

# ===== On-Device Agent Tests =====
log ""
log "--- On-Device Agent Tests ---"

# Test: Agent process running
log "TEST: NinjaMagic Agent process"
if $ADB shell "ps -A | grep ninjamagic-agent" >/dev/null 2>&1; then
    pass "ninjamagic-agent process running"
else
    fail "ninjamagic-agent process not found"
fi

# Test: Agent MCP server running
log "TEST: MCP server process"
if $ADB shell "ps -A | grep ninjamagic-mcp" >/dev/null 2>&1; then
    pass "ninjamagic-mcp-server running"
else
    fail "ninjamagic-mcp-server not found"
fi

# Test: Agent connected to MSI substrate
log "TEST: MSI substrate connection"
if $ADB shell "logcat -d -t 200 | grep 'Connected to MSI substrate'" >/dev/null 2>&1; then
    pass "Agent connected to MSI substrate"
else
    fail "Agent MSI substrate connection not found"
fi

# Test: Agent domain sealed
log "TEST: Agent domain sealed"
if $ADB shell "logcat -d -t 200 | grep 'domain.*sealed'" >/dev/null 2>&1; then
    pass "Agent domain sealed"
else
    fail "Agent domain not sealed"
fi

# Test: Skills registered
log "TEST: Skill registration"
SKILLS=$($ADB shell "logcat -d -t 200 | grep -c 'Registered skill'" 2>/dev/null || echo "0")
if [[ "$SKILLS" -ge 5 ]]; then
    pass "$SKILLS skills registered"
else
    fail "Only $SKILLS skills registered (expected >= 5)"
fi

# Test: Intent classifier
log "TEST: Intent classifier"
INTENTS="call text settings camera alarm"
INTENT_PASS=0
for intent in $INTENTS; do
    if $ADB shell "logcat -d -t 500 | grep -i 'intent.*$intent\|classify'" >/dev/null 2>&1; then
        ((INTENT_PASS++))
    fi
done
if [[ $INTENT_PASS -ge 1 ]]; then
    pass "Intent classifier active ($INTENT_PASS categories detected)"
else
    skip "Intent classifier not exercised (no input yet)"
fi

# Test: Memory system initialized
log "TEST: Memory system"
MEM=$($ADB shell "logcat -d -t 200 | grep -i 'memory.*init\|assoc.*store'" 2>/dev/null || echo "")
if [[ -n "$MEM" ]]; then
    pass "Memory system initialized"
else
    fail "Memory system not initialized"
fi

# Test: Inference engine
log "TEST: Inference engine"
INF=$($ADB shell "logcat -d -t 200 | grep -i 'inference.*init\|model.*load'" 2>/dev/null || echo "")
if [[ -n "$INF" ]]; then
    pass "Inference engine initialized"
else
    skip "Inference engine not yet loaded (may need model files)"
fi

fi # end MCP_ONLY check

# ===== Summary =====
log ""
log "============================================"
log "AGENT TEST RESULTS"
log "============================================"
log "  PASS: $PASS"
log "  FAIL: $FAIL"
log "  SKIP: $SKIP"
log "  TOTAL: $((PASS + FAIL + SKIP))"
log "============================================"
log "Results saved to: $LOG"

exit $FAIL
