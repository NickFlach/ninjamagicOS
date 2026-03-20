# MSI v1.0 Specification (ninjamagicOS Edition)

This directory contains the MSI (Minimal Substrate Interface) specification adapted for ninjamagicOS.

The canonical MSI spec lives in `Source/SingularisPrime/msi/`. This directory contains:

- Phone-specific MSI extensions (NPU affinity, radio events, sensor topics)
- Hardware capability profiles for Pixel 7 (Tensor GS201) and Nord N30 (SD695)
- MSI bytecode format specification for on-device SP program execution

## Phone-Specific MSI Extensions

### Additional Affinity Targets
```
Affinity ::= "any" | "little" | "big" | "npu" | "gpu" | "dsp"
```

- `npu` → Tensor TPU (Pixel) or Hexagon 686 (Nord)
- `gpu` → Mali-G710 (Pixel) or Adreno 619 (Nord) for compute shaders
- `dsp` → Hexagon DSP (Nord only, Pixel uses TPU)

### Reserved Event Topic Namespaces
```
phone/call/*       — Telephony call events
phone/sms/*        — SMS events
phone/data/*       — Mobile data events
phone/signal/*     — Signal strength
phone/network/*    — Network registration
sensor/accel/*     — Accelerometer
sensor/gyro/*      — Gyroscope
sensor/gps/*       — Location
sensor/proximity/* — Proximity sensor
sensor/light/*     — Ambient light
sensor/fingerprint/* — Fingerprint events
camera/*           — Camera events
audio/*            — Audio events
power/*            — Battery/charging events
agent/*            — Agent lifecycle events
agent/intent/*     — User intent classification
agent/skill/*      — Skill execution events
agent/memory/*     — Memory operations
ui/*               — UI state events
system/*           — System events (boot, shutdown, etc.)
```

### Hardware Capability Profiles

#### Pixel 7 (Tensor GS201)
```yaml
caps:
  lanes:
    min: 1
    max: 1024
    realtime: true
  events:
    model: topic
    maxTopics: 65536
  state:
    model: hybrid
    maxBytes: 8589934592  # 8GB
  clock:
    model: monotonic
  security:
    model: tee  # Titan M2 + Trusty TEE
    attest: true
  accel:
    cpu: true
    gpu: true   # Mali-G710
    npu: true   # Google TPU
    dsp: false
```

#### Nord N30 (Snapdragon 695)
```yaml
caps:
  lanes:
    min: 1
    max: 512
    realtime: true
  events:
    model: topic
    maxTopics: 32768
  state:
    model: hybrid
    maxBytes: 8589934592  # 8GB
  clock:
    model: monotonic
  security:
    model: app-sandbox  # Qualcomm SPU
    attest: true
  accel:
    cpu: true
    gpu: true   # Adreno 619
    npu: false  # No dedicated NPU
    dsp: true   # Hexagon 686
```
