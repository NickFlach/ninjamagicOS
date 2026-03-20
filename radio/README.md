# ninjamagicOS Radio Interface Layer

Custom RILD (Radio Interface Layer Daemon) that wraps vendor modem binaries
and bridges telephony events to the MSI event bus.

## Architecture

```
┌───────────────────────────────────────────────┐
│        NinjaMagic Agent                        │
│  (subscribes to phone/* MSI events)           │
├───────────────────────────────────────────────┤
│        NinjaMagic Telephony Service            │
│  (MSI lane: phone management, call state)     │
├───────────────────────────────────────────────┤
│        MSI Event Bridge                        │
│  (translates RIL callbacks → MSI events)      │
├───────────────────────────────────────────────┤
│        NinjaMagic RILD                         │
│  (custom daemon, loads vendor RIL .so)        │
├───────────────────────────────────────────────┤
│        LibRIL (AOSP standard)                  │
├───────────────────────────────────────────────┤
│        Vendor RIL Library (.so)                │
│  Pixel 7: Samsung Exynos 5300 RIL             │
│  Nord N30: Qualcomm Snapdragon X51 RIL        │
├───────────────────────────────────────────────┤
│        Modem Firmware (Baseband)               │
│  (flashed separately, not rebuilt)            │
└───────────────────────────────────────────────┘
```

## MSI Event Topics

All telephony state is published on the MSI event bus:

| Topic | Payload | Direction |
|-------|---------|-----------|
| `phone/call/incoming` | `{number, name, slot}` | modem → agent |
| `phone/call/outgoing` | `{number, slot}` | agent → modem |
| `phone/call/active` | `{call_id, number, duration}` | modem → agent |
| `phone/call/ended` | `{call_id, reason, duration}` | modem → agent |
| `phone/call/dial` | `{number}` | agent → RILD |
| `phone/call/answer` | `{call_id}` | agent → RILD |
| `phone/call/hangup` | `{call_id}` | agent → RILD |
| `phone/sms/received` | `{from, body, timestamp}` | modem → agent |
| `phone/sms/sent` | `{to, body, status}` | RILD → agent |
| `phone/sms/send` | `{to, body}` | agent → RILD |
| `phone/data/connected` | `{type, apn, ip}` | modem → agent |
| `phone/data/disconnected` | `{reason}` | modem → agent |
| `phone/signal/strength` | `{rssi, rsrp, rsrq, snr}` | modem → agent |
| `phone/network/registered` | `{operator, type, roaming}` | modem → agent |
| `phone/sim/state` | `{slot, state, iccid}` | modem → agent |

## Files

```
radio/
├── rild/                   # Custom RILD daemon
│   ├── main.cpp            # RILD entry point
│   ├── msi_bridge.cpp      # RIL callback → MSI event translator
│   ├── msi_bridge.h
│   ├── vendor_ril.cpp      # Vendor .so loader
│   └── vendor_ril.h
├── telephony/              # Telephony service (MSI lane)
│   ├── service.rs          # Main telephony service
│   ├── call_manager.rs     # Call state machine
│   └── sms_manager.rs      # SMS send/receive
└── vendor/                 # Vendor RIL extraction scripts
    ├── extract_pixel7.sh   # Extract from Pixel 7 factory image
    └── extract_nordn30.sh  # Extract from Nord N30 firmware
```
