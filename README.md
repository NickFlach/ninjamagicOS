# ninjamagicOS

**An AI-native mobile operating system built from scratch.**

ninjamagicOS is a purpose-built Android-based OS where every layer — from bootloader to radio to UI shell — is designed around a native AI agent runtime. The phone's intelligence isn't an app; it's the operating system.

## Target Devices

| Device | SoC | Codename | Status |
|--------|-----|----------|--------|
| Google Pixel 7 | Tensor GS201 | panther | 🔴 Planning |
| OnePlus Nord N30 5G | Snapdragon 695 | larry | 🔴 Planning |

## Core Innovations

### MSI Cognitive Runtime
The Minimal Substrate Interface (MSI) from [SingularisPrime](../SingularisPrime/) runs as a kernel module, providing cognitive execution primitives:
- **Lanes** — AI-optimized execution contexts with priority/energy/affinity policies
- **Events** — Zero-latency topic-based pub/sub connecting all phone subsystems
- **AssocStore** — Hardware-accelerated associative (vector) memory on NPU/GPU
- **Domains** — Capability-sealed security containers (no ambient authority)

### NinjaMagic Agent
An always-on AI agent inspired by [OpenClaw](https://openclaw.ai/), running natively as an MSI lane:
- On-device LLM inference (quantized models on TPU/Hexagon DSP)
- Direct access to telephony, camera, sensors, files — not through app APIs
- Extensible skill system written in Shinobi.Substrate (SP) language
- MCP server exposing phone capabilities as tools
- Four-tier memory: working, episodic, semantic, procedural

### Capability-Based Security
From [QuantumOS](../QuantumOS/), every app and agent skill runs in a sealed domain:
- Explicit capability grants (not blanket permissions)
- Hardware attestation via Titan M2 / Qualcomm SPU
- Immutable sealing prevents privilege escalation

### Biofield-Aware UX
From [Space-Child-Dream](../Space-Child-Dream/), the UI adapts to your physiological state:
- Wearable integration (BLE heart rate, HRV)
- UI tempo/color shifts based on biofield state
- Consciousness graph identity (not social metrics)

## Architecture

```
┌─────────────────────────────────────────────────┐
│           NinjaMagic Agent Layer                 │
│   Agent Core │ Skills │ MCP │ On-Device LLM     │
├─────────────────────────────────────────────────┤
│           MSI Cognitive Runtime                  │
│   Lanes │ Events │ AssocStore │ Domains          │
├─────────────────────────────────────────────────┤
│           Frameworks & Services                  │
│   Telephony │ Display │ Audio │ Sensors          │
├─────────────────────────────────────────────────┤
│           Hardware Abstraction Layer             │
│   Radio │ GPU/NPU │ Camera │ Sensor HALs        │
├─────────────────────────────────────────────────┤
│           Kernel (Linux 6.1/5.4 + MSI module)   │
│   Resonant Scheduler │ Capabilities │ Memory    │
├─────────────────────────────────────────────────┤
│           Bootloader / Firmware                  │
│   ABL/XBL │ Radio Firmware │ TEE │ Secure Boot  │
└─────────────────────────────────────────────────┘
```

## Repository Structure

```
ninjamagicOS/
├── bootloader/          # Custom boot image configuration
├── kernel/              # Kernel modules and patches
│   └── drivers/msi/     # MSI kernel module
├── hal/                 # Hardware Abstraction Layer
│   ├── common/          # Shared HAL interfaces
│   ├── tensor/          # Google Tensor GS201 backends
│   └── snapdragon/      # Qualcomm SD695 backends
├── radio/               # Radio Interface Layer (RIL)
├── frameworks/          # System framework services
├── system/              # Core system components
├── packages/            # Built-in applications
├── device/              # Device-specific configs
│   ├── google/panther/  # Pixel 7
│   └── oneplus/larry/   # Nord N30
├── vendor/              # Proprietary vendor blobs
├── agent/               # NinjaMagic AI Agent
│   ├── core/            # Agent core (Rust/Kotlin)
│   ├── skills/          # Built-in skills (SP programs)
│   └── mcp/             # MCP server/client
├── msi/                 # MSI Cognitive Runtime
│   ├── spec/            # MSI v1.0 specification
│   ├── runtime/         # Userspace MSI runtime (Rust)
│   └── compiler/        # SP → MSI bytecode compiler
├── tools/               # Build and flash tools
├── build/               # Build system configuration
├── tests/               # Test suites
└── docs/                # Documentation
    ├── adr/             # Architecture Decision Records
    └── PROJECT_PLAN.md  # Phased project plan
```

## Source Directory Innovations

ninjamagicOS integrates battle-tested innovations from across the Source monorepo:

| Component | Source | Used For |
|-----------|--------|----------|
| MSI v1.0 | SingularisPrime | Cognitive execution primitives |
| Shinobi.Substrate | SingularisPrime | Agent skill programming language |
| Capability Security | QuantumOS | App/skill sandboxing |
| Resonant Scheduler | QuantumOS | AI-aware process scheduling |
| Space Child Auth | Space-Child-Dream | User identity |
| Biofield Profile | Space-Child-Dream | Physiological UI adaptation |
| Wearable Integration | Space-Child-Dream | BLE sensor data |
| GooseNeutron | SpaceChildDev | Agent architecture patterns |

## Getting Started

> ⚠️ **ninjamagicOS is in early planning stage.** Build instructions will be added as development progresses.

### Prerequisites
- Ubuntu 22.04+ or WSL2
- 64+ GB RAM, 500+ GB disk
- Android SDK Platform Tools
- Rust nightly toolchain
- Android NDK r26+
- Target device (Pixel 7 or Nord N30, unlocked bootloader)

## Documentation

- [Architecture Decision Record](docs/adr/001-architecture-decision-record.md) — Why we made the choices we made
- [Project Plan](docs/PROJECT_PLAN.md) — Phased development roadmap

## License

TBD — Dual license under consideration (GPL v2 for kernel components, Apache 2.0 for userspace)

## Space Child Ecosystem

ninjamagicOS is the **Space Child Phone** — `phone.spacechild.love`

Part of the [Space Child](https://spacechild.love) ecosystem of AI-native applications.
