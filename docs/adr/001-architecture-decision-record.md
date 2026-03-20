# ADR-001: ninjamagicOS Architecture Decision Record

**Status:** Accepted  
**Date:** 2026-03-19  
**Authors:** Nick Flach  
**Supersedes:** N/A

---

## 1. Context & Motivation

We are building **ninjamagicOS** — a from-scratch Android-based operating system targeting mobile phones. This is not a custom ROM or AOSP fork in the traditional sense. Every layer of the stack — from bootloader through radio interface to the AI agent runtime — will be purpose-built to optimize for AI-native operation, leveraging innovations already developed across the Source directory.

### 1.1 Why Build From Scratch?

- **AI-First Primitives**: Existing Android distributions treat AI as an application-layer concern. We treat it as a kernel-level primitive via MSI (Minimal Substrate Interface) from SingularisPrime.
- **Cognitive Execution Model**: Traditional thread/process models are wasteful for always-on AI. MSI's Lanes, Events, and Associative Memory provide a cognitive-native execution substrate.
- **Unified Agent**: Instead of bolting an assistant app on top of Android, the OS itself IS the agent. The phone's native intelligence is not an app — it's the runtime.
- **Hardware Optimization**: By controlling every layer, we can optimize NPU/GPU/DSP utilization for on-device inference in ways stock Android cannot.
- **Privacy by Architecture**: On-device processing, capability-based security from QuantumOS, and sealed domains ensure user data never leaves the device unnecessarily.

### 1.2 Target Hardware

#### Google Pixel 7 (codename: panther)
| Component | Specification |
|-----------|---------------|
| **SoC** | Google Tensor GS201 (Samsung Exynos-based) |
| **CPU** | 2x Cortex-X1 @ 2.85GHz + 2x Cortex-A78 @ 2.35GHz + 4x Cortex-A55 @ 1.8GHz |
| **GPU** | ARM Mali-G710 MC10 |
| **NPU** | Google TPU (Tensor Processing Unit) |
| **Modem** | Samsung Exynos 5300 (5G NR, LTE, UMTS, GSM) |
| **RAM** | 8 GB LPDDR5 |
| **Storage** | 128/256 GB UFS 3.1 |
| **Security** | Titan M2 coprocessor, TEE (Trusty) |
| **Display** | 6.3" AMOLED 1080x2400 @ 90Hz |
| **Kernel Branch** | android-gs-pantah-6.1 |
| **Bootloader** | Unlockable via OEM unlock |
| **Partitions** | boot, dtbo, vendor_kernel_boot, vendor_dlkm, system_dlkm |

#### OnePlus Nord N30 5G (codename: larry)
| Component | Specification |
|-----------|---------------|
| **SoC** | Qualcomm Snapdragon 695 (SM6375) |
| **CPU** | 2x Kryo 660 Gold @ 2.2GHz + 6x Kryo 660 Silver @ 1.7GHz |
| **GPU** | Qualcomm Adreno 619 |
| **NPU** | Qualcomm Hexagon 686 DSP |
| **Modem** | Snapdragon X51 (5G NR, LTE, UMTS, GSM) |
| **RAM** | 8 GB LPDDR4X |
| **Storage** | 128 GB UFS 2.2 + microSD up to 1TB |
| **Security** | Qualcomm SPU |
| **Display** | 6.72" IPS LCD 1080x2400 @ 120Hz |
| **Kernel Branch** | kernel 5.4 (LineageOS: android16 supported) |
| **Bootloader** | Volume Up + Volume Down + Power → Fastboot |
| **Supported Models** | CPH2513 |

### 1.3 Source Directory Innovations to Integrate

| Innovation | Source Repo | Integration Point |
|-----------|-------------|-------------------|
| **MSI v1.0** (Minimal Substrate Interface) | SingularisPrime | Kernel HAL — cognitive execution primitives |
| **Shinobi.Substrate** (SP Language) | SingularisPrime | Agent runtime — cognitive program language |
| **MSI Bytecode** | SingularisPrime | Portable cognitive program distribution |
| **Capability-Based Security** | QuantumOS | Domain/grant model for app sandboxing |
| **Resonant Scheduler** | QuantumOS | AI-aware process scheduling with coherence |
| **Microkernel IPC** | QuantumOS | Minimal trusted kernel, user-space services |
| **Space Child Auth** | Space-Child-Dream | User identity and cross-device authentication |
| **Biofield Profile** | Space-Child-Dream | Physiological-aware UI adaptation |
| **Wearable Integration** | Space-Child-Dream | BLE/cloud wearable data for biofield |
| **GooseNeutron Agent** | SpaceChildDev | AI agent architecture (tool calls, MCP) |
| **0xSCADA Blockchain** | 0xSCADA | On-chain attestation, identity anchoring |
| **Signal Processing** | Space-Child-Dream | Sensor → biofield state translation |

---

## 2. Decision: Architecture Overview

### 2.1 Layer Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    NinjaMagic Agent Layer                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐   │
│  │  Agent   │ │  Skills  │ │   MCP    │ │  On-Device LLM   │   │
│  │  Core    │ │  Engine  │ │  Server  │ │  Inference (NPU)  │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                    MSI Cognitive Runtime                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐   │
│  │  Lanes   │ │  Events  │ │  Assoc   │ │    Domains       │   │
│  │ (exec)   │ │ (pubsub) │ │ (memory) │ │   (security)     │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                    Frameworks & Services                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐   │
│  │Telephony │ │ Display  │ │  Audio   │ │    Sensors       │   │
│  │ Service  │ │  Server  │ │  Server  │ │    Service       │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                    Hardware Abstraction Layer                    │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐   │
│  │  Radio   │ │  GPU/NPU │ │  Camera  │ │   Sensors HAL    │   │
│  │  HAL     │ │   HAL    │ │   HAL    │ │   (IMU, etc.)    │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                    Kernel Layer                                  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐   │
│  │ Resonant │ │ Capability│ │  Memory  │ │    MSI Kernel    │   │
│  │Scheduler │ │  System  │ │  Manager │ │    Module        │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                    Bootloader / Firmware                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐   │
│  │  ABL /   │ │  Radio   │ │  TrustZone│ │   Secure Boot   │   │
│  │  U-Boot  │ │ Firmware │ │   / TEE  │ │   Chain          │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                         HARDWARE
  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐
  │ Tensor   │ │ Snapdragon│ │  Modem   │ │   Sensors /      │
  │  GS201   │ │   695    │ │ Baseband │ │   Peripherals    │
  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘
```

### 2.2 Core Design Decisions

#### Decision 2.2.1: AOSP as Foundation, Not Fork
**Choice:** Start from AOSP source and rebuild every layer with our innovations.  
**Rationale:** Building a phone OS truly from scratch (bare metal up) would take years for radio/telephony alone. AOSP provides a tested foundation for hardware interaction. We replace/extend every major subsystem while maintaining compatibility with Android app ecosystem where needed.  
**Trade-off:** We accept AOSP's Linux kernel base but inject our cognitive primitives as kernel modules and HAL extensions.

#### Decision 2.2.2: MSI as the Cognitive HAL
**Choice:** Implement SingularisPrime's MSI v1.0 as a native kernel module and HAL service.  
**Rationale:** MSI's Lanes/Events/Domains/Assoc primitives are designed exactly for cognitive workloads. By implementing MSI at the kernel level (not just as an Android app), we get:
- Realtime lane scheduling on big/little/NPU cores via `LanePolicy.affinity`
- Zero-copy event delivery between cognitive programs
- Hardware-backed associative memory using NPU for vector operations
- Capability-sealed domains for cognitive program isolation

**Implementation:**
- `kernel/drivers/msi/` — Linux kernel module implementing MSI primitives
- `hal/msi/` — Android HAL service exposing MSI to userspace
- `msi/runtime/` — Userspace MSI runtime (Rust + C)
- `msi/compiler/` — SP → MSI bytecode compiler

#### Decision 2.2.3: Native AI Agent (NinjaMagic Agent)
**Choice:** Build an always-on AI agent inspired by OpenClaw, running as a first-class OS service via MSI lanes.  
**Rationale:** OpenClaw demonstrates the power of an always-on AI assistant that can actually do things. By making this native to the OS (not an app), we gain:
- Direct access to all phone subsystems (telephony, camera, sensors, files)
- NPU-accelerated on-device inference (no cloud dependency for basic operations)
- MSI event-driven architecture for zero-latency context awareness
- Sealed domains for privacy-preserving skill execution

**Agent Architecture:**
```
NinjaMagic Agent
├── Core Loop (MSI Lane: priority=high, affinity=big)
│   ├── Context Aggregator (subscribes to sensor/*, phone/*, user/* events)
│   ├── Intent Router (classifies user intent → skill dispatch)
│   └── Response Generator (on-device LLM inference)
├── Skill Engine (MSI Lanes: priority=normal)
│   ├── Phone Skills (calls, SMS, contacts)
│   ├── System Skills (settings, apps, files)
│   ├── Web Skills (browser, search, API calls)
│   ├── Creative Skills (camera, audio, generation)
│   └── Custom Skills (user-defined via SP programs)
├── MCP Server (MSI Lane: priority=normal)
│   ├── Tool Registry (phone capabilities as MCP tools)
│   ├── Resource Provider (phone state as MCP resources)
│   └── External MCP Client (connect to remote MCP servers)
├── Memory System (MSI Assoc spaces)
│   ├── Working Memory (current context, active tasks)
│   ├── Episodic Memory (conversation history, interactions)
│   ├── Semantic Memory (user preferences, learned patterns)
│   └── Procedural Memory (skill execution traces)
└── On-Device Inference
    ├── Primary: Quantized LLM (4-bit GGUF on NPU/GPU)
    ├── Fallback: Cloud API (user-configurable provider)
    ├── Embedding: MiniLM for assoc memory vectors
    └── Classification: Tiny models for intent routing
```

#### Decision 2.2.4: Dual-SoC Hardware Abstraction
**Choice:** Abstract SoC differences behind a unified HAL with per-device backends.  
**Rationale:** Tensor GS201 and Snapdragon 695 have fundamentally different architectures:
- Tensor has a custom TPU for ML; Snapdragon has Hexagon DSP
- Different modem vendors (Samsung Exynos vs Qualcomm X51)
- Different GPU architectures (Mali vs Adreno)
- Different kernel branches and driver models

**Implementation:**
```
hal/
├── common/              # Shared HAL interfaces
│   ├── INinjaMagicRadio.h
│   ├── INinjaMagicNPU.h
│   ├── INinjaMagicGPU.h
│   ├── INinjaMagicSensor.h
│   └── INinjaMagicCamera.h
├── tensor/              # Google Tensor GS201 backends
│   ├── radio/           # Exynos 5300 modem RIL
│   ├── npu/             # Google TPU HAL
│   ├── gpu/             # Mali-G710 HAL
│   └── camera/          # Pixel camera HAL
└── snapdragon/          # Qualcomm SD695 backends
    ├── radio/           # Snapdragon X51 modem RIL
    ├── npu/             # Hexagon 686 DSP HAL
    ├── gpu/             # Adreno 619 HAL
    └── camera/          # OnePlus camera HAL
```

#### Decision 2.2.5: Radio Interface Layer (RIL)
**Choice:** Custom RIL implementation wrapping vendor modem binaries with MSI event integration.  
**Rationale:** The radio/modem is the most critical and complex phone component. Vendor modem firmware is proprietary and cannot be replaced. We build a custom RILD that:
- Wraps vendor RIL libraries (Samsung for Pixel 7, Qualcomm for Nord N30)
- Exposes telephony events via MSI event bus (`phone/call/*`, `phone/sms/*`, `phone/data/*`)
- Enables the AI agent to interact with telephony natively
- Supports VoLTE, VoWiFi, 5G NR through vendor IMS stacks

**RIL Architecture:**
```
┌──────────────────────────────────┐
│     NinjaMagic Telephony         │
│  (MSI Lane: phone/* events)     │
├──────────────────────────────────┤
│     NinjaMagic RILD              │
│  (Custom daemon, MSI-integrated) │
├──────────────────────────────────┤
│     LibRIL (AOSP standard)       │
├──────────────────────────────────┤
│     Vendor RIL .so               │
│  (Samsung / Qualcomm binary)     │
├──────────────────────────────────┤
│     Modem Firmware (Baseband)    │
│  (Exynos 5300 / Snapdragon X51) │
└──────────────────────────────────┘
```

#### Decision 2.2.6: Bootloader Strategy
**Choice:** Custom bootloader chain building on device-specific primary bootloaders.  
**Rationale:** 
- **Pixel 7**: Uses Android Bootloader (ABL) which is unlockable. We flash custom boot/vendor_kernel_boot images via fastboot. The primary bootloader (PBL) and ABL stay intact for hardware initialization.
- **Nord N30**: Uses Qualcomm's standard boot chain. Unlockable via OEM settings. We flash custom boot images.

Both devices support verified boot which we'll re-sign with our own keys for production builds, while using unlocked bootloaders for development.

**Boot Sequence:**
```
1. Primary Bootloader (PBL) — vendor, immutable
2. Secondary Bootloader (ABL/XBL) — vendor, unlocked for dev
3. Linux Kernel + NinjaMagic kernel modules
4. MSI Substrate Initialization (contract.yaml boot sequence)
   a. Substrate Probe — query hardware capabilities
   b. State Initialization — map core memory regions
   c. Event System — initialize MSI event bus
   d. Cognitive Bootstrap — spawn kernel lanes
5. Android Init → NinjaMagic System Services
6. NinjaMagic Agent launch (always-on MSI lane)
```

#### Decision 2.2.7: Capability-Based Security Model
**Choice:** Adopt QuantumOS's capability-based security as the foundation, extended with MSI domains.  
**Rationale:** Traditional Android permissions are coarse-grained and user-hostile. Our model:
- Every app/agent-skill runs in a sealed MSI Domain
- Capabilities are explicit grants: `Events("sensor/gps")`, `State("contacts", r)`, `Accel("npu")`
- Domains can be sealed (immutable permissions) — no privilege escalation
- Hardware attestation via Titan M2 (Pixel) / Qualcomm SPU (Nord)
- AI agent skills get minimal capabilities by default

#### Decision 2.2.8: On-Device LLM Strategy
**Choice:** Ship quantized open-weight LLMs optimized for each SoC's accelerator.  
**Rationale:**
- **Pixel 7 TPU**: Google's TPU excels at transformer inference. Target 4-bit quantized models (3B-7B parameter range) running on TPU with Mali-G710 fallback.
- **Nord N30 Hexagon DSP**: Qualcomm's AI Engine supports ONNX/TFLite. Target 4-bit quantized models (1B-3B parameter range) with Adreno 619 GPU compute fallback.
- **Model Options**: Llama 3.2 1B/3B, Phi-3 Mini, Gemma 2B — all with tool-calling fine-tunes
- **Embedding Model**: all-MiniLM-L6-v2 for MSI AssocStore vector operations

#### Decision 2.2.9: Space Child Ecosystem Integration
**Choice:** Integrate Space Child Auth, Biofield Profile, and Wearable connectivity natively.  
**Rationale:** ninjamagicOS is the "Space Child Phone" (`phone.spacechild.love`). Native integration means:
- Space Child Auth for user identity (no separate login)
- Biofield profile drives UI adaptation (tempo, color, intensity based on physiological state)
- Wearable data feeds directly into MSI events for agent context
- Published artifacts sync with Space-Child-Dream

---

## 3. Technical Decisions Summary

| # | Decision | Choice | Confidence |
|---|----------|--------|------------|
| 2.2.1 | Foundation | AOSP base, rebuild every layer | High |
| 2.2.2 | Cognitive HAL | MSI v1.0 as kernel module + HAL | High |
| 2.2.3 | AI Agent | Native always-on agent via MSI lanes | High |
| 2.2.4 | Multi-SoC | Unified HAL with per-device backends | High |
| 2.2.5 | Radio | Custom RILD wrapping vendor binaries | High |
| 2.2.6 | Bootloader | Custom boot images, vendor PBL intact | High |
| 2.2.7 | Security | Capability-based with MSI domains | High |
| 2.2.8 | On-Device AI | Quantized LLMs on NPU/GPU per SoC | Medium |
| 2.2.9 | Ecosystem | Space Child Auth + Biofield native | High |

---

## 4. Risks & Mitigations

| Risk | Severity | Mitigation |
|------|----------|------------|
| **Vendor binary dependencies** — Modem, GPU, and DSP drivers are proprietary blobs | Critical | Extract from factory images; wrap via HAL; keep vendor partition intact |
| **AOSP source availability** — Google is restricting AOSP source (2025-2026 changes) | High | Use `android-latest-release` branch; maintain own patch sets; LineageOS as reference |
| **NPU inference performance** — On-device LLM may be too slow for real-time agent | High | Tiered approach: fast classifier for routing, larger model for generation; cloud fallback |
| **Radio stability** — Custom RILD may cause call/data issues | Critical | Extensive RIL testing; keep vendor RIL .so unchanged; wrap don't replace |
| **Secure boot re-signing** — Production devices need verified boot | Medium | Development on unlocked bootloaders; AVB signing for release builds |
| **Battery drain** — Always-on AI agent | High | MSI energy budgets; lane sleep; NPU efficiency; biofield-aware throttling |
| **Two-SoC maintenance burden** — Supporting different architectures | Medium | Strong HAL abstraction; shared 90% of codebase; device-specific only in hal/ |

---

## 5. Technology Stack

| Layer | Technology | Language |
|-------|-----------|----------|
| **Bootloader** | ABL/XBL (vendor) + custom boot images | N/A (binary) |
| **Kernel** | Linux 6.1 (Pixel) / 5.4 (Nord) + custom modules | C, Assembly |
| **MSI Kernel Module** | Linux kernel module | C |
| **MSI Runtime** | Userspace MSI implementation | Rust, C |
| **SP Compiler** | Shinobi.Substrate → MSI bytecode | Rust |
| **HAL** | Android HIDL/AIDL HAL interfaces | C++, Rust |
| **Radio/RIL** | Custom RILD + vendor libs | C++, C |
| **Framework Services** | Android system services (rebuilt) | Kotlin, Java |
| **Agent Core** | NinjaMagic Agent | Rust, Kotlin |
| **Agent Skills** | Skill programs in SP language | SP (Shinobi.Substrate) |
| **On-Device LLM** | llama.cpp / ONNX Runtime | C++, Rust |
| **UI Shell** | Custom launcher + system UI | Kotlin, Jetpack Compose |
| **Build System** | Soong (Android) + custom Ninja scripts | Blueprint, Make |

---

## 6. Consequences

### Positive
- First truly AI-native phone OS with cognitive primitives at kernel level
- Privacy-first: on-device inference, capability-sealed domains
- Unified agent experience across all phone functions
- Leverages years of Source directory R&D (MSI, QuantumOS, Space Child)
- Two diverse hardware targets prove architecture portability

### Negative
- Massive engineering scope — every layer must be built/adapted
- Vendor binary dependencies create fragility
- Two different SoC architectures doubles HAL work
- Ongoing maintenance as AOSP and vendor firmware evolve
- On-device LLM quality limited by phone hardware

### Neutral
- Android app compatibility maintained through AOSP framework layer
- Space Child ecosystem integration creates both value and coupling
- Open-source approach enables community but requires governance

---

## 7. References

- [SingularisPrime MSI Spec](../../msi/spec/) — Cognitive HAL specification
- [QuantumOS Kernel](../../../QuantumOS/) — Capability security & resonant scheduler
- [AOSP Build Documentation](https://source.android.com/docs/setup/build)
- [Pixel 7 Kernel Source](https://source.android.com/docs/setup/build/building-pixel-kernels) — Branch: android-gs-pantah-6.1
- [LineageOS Nord N30 (larry)](https://wiki.lineageos.org/devices/larry/) — Reference for device tree
- [Android RIL Architecture](https://source.android.com/docs/core/connect/ril)
- [OpenClaw](https://openclaw.ai/) — AI agent architecture inspiration
- [Space-Child-Dream Auth](../../../Space-Child-Dream/) — Authentication system
