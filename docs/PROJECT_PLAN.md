# ninjamagicOS — Project Plan

**Version:** 0.1.0  
**Last Updated:** 2026-03-19

---

## Executive Summary

ninjamagicOS is an AI-native mobile operating system built from the ground up on AOSP foundations, targeting Google Pixel 7 and OnePlus Nord N30 5G. Every layer — bootloader, kernel, radio, HAL, frameworks, and shell — is purpose-built to run a native AI agent (NinjaMagic Agent) as a first-class OS citizen, leveraging the MSI cognitive runtime from SingularisPrime and capability-based security from QuantumOS.

---

## Phase 0: Foundation & Build Environment (Weeks 1–4)

### 0.1 Build Infrastructure
- [ ] Set up AOSP build environment (Ubuntu 22.04/24.04 or WSL2)
- [ ] Download AOSP source (`android-latest-release` or `android16-release`)
- [ ] Download Pixel 7 (panther) proprietary binaries (Google + Qualcomm sets)
- [ ] Download Nord N30 (larry) vendor blobs (extract from factory/LineageOS)
- [ ] Verify vanilla AOSP builds and boots on both devices
- [ ] Set up CI/CD pipeline (GitHub Actions with self-hosted ARM64 runners)
- [ ] Configure signing keys (test keys for dev, AVB keys for release)

### 0.2 Device Trees
- [ ] Create `device/google/panther/` device tree (BoardConfig, device.mk, kernel prebuilt)
- [ ] Create `device/oneplus/larry/` device tree (BoardConfig, device.mk, kernel prebuilt)
- [ ] Verify both device trees produce bootable images from AOSP
- [ ] Document partition layouts for both devices
- [ ] Create `vendor/` extraction scripts for both devices

### 0.3 Kernel Preparation
- [ ] Clone Pixel 7 kernel (`android-gs-pantah-6.1-android16`)
- [ ] Clone/build Nord N30 kernel (kernel 5.4, LineageOS reference)
- [ ] Verify both kernels boot with stock AOSP userspace
- [ ] Identify kernel config options needed for MSI module
- [ ] Enable kernel module loading for custom modules

**Milestone: Both devices boot stock AOSP from our build system**

---

## Phase 1: MSI Kernel Integration (Weeks 5–10)

### 1.1 MSI Kernel Module
- [ ] Port MSI v1.0 spec to Linux kernel module (`kernel/drivers/msi/`)
- [ ] Implement MSI Lane primitives (backed by kernel threads with CFS/RT scheduling)
- [ ] Implement MSI Event Bus (kernel-space topic pub/sub with netlink to userspace)
- [ ] Implement MSI State regions (mmap-backed byte buffers with commit semantics)
- [ ] Implement MSI Domain manager (capability grants, sealing, attestation stubs)
- [ ] Implement MSI Clock (monotonic nanosecond clock, event-based clock option)
- [ ] Write kernel module tests (kunit or custom test framework)
- [ ] Verify module loads on both Pixel 7 and Nord N30 kernels

### 1.2 MSI Userspace Runtime
- [ ] Build MSI userspace library in Rust (`msi/runtime/`)
- [ ] Implement ioctl/netlink interface to kernel module
- [ ] Implement AssocStore (userspace vector database using HNSW or IVF-PQ)
- [ ] NPU-accelerated vector operations for AssocStore (Tensor TPU / Hexagon DSP)
- [ ] Expose MSI runtime as Android system service (AIDL interface)
- [ ] Write integration tests: Lane spawn/kill, Event pub/sub, Assoc put/query

### 1.3 SP Compiler (Minimal)
- [ ] Implement SP lexer/parser in Rust (`msi/compiler/`)
- [ ] Implement type checker for domain/lane/grant validation
- [ ] Implement lowering pass (SP AST → MSI bytecode)
- [ ] Implement MSI bytecode interpreter in the runtime
- [ ] Compile and run `hello-substrate` example from SingularisPrime

**Milestone: MSI cognitive primitives running on both devices, SP programs execute**

---

## Phase 2: Radio & Telephony (Weeks 7–14)

### 2.1 Radio Interface Layer
- [ ] Extract vendor RIL .so from both devices' factory images
- [ ] Build custom RILD (`radio/rild/`) that loads vendor RIL
- [ ] Implement LibRIL wrapper with MSI event integration
- [ ] Map RIL solicited/unsolicited responses to MSI events:
  - `phone/call/incoming`, `phone/call/outgoing`, `phone/call/ended`
  - `phone/sms/received`, `phone/sms/sent`
  - `phone/data/connected`, `phone/data/disconnected`
  - `phone/signal/strength`, `phone/network/registered`
- [ ] Test basic calls and SMS on both devices
- [ ] Test mobile data connectivity (LTE/5G)
- [ ] Test VoLTE/VoWiFi (requires vendor IMS stack)

### 2.2 Modem Firmware
- [ ] Extract modem firmware from factory images (Exynos 5300 / SD X51)
- [ ] Document modem partition flash procedures for both devices
- [ ] Verify modem firmware version compatibility with our RILD
- [ ] Create modem firmware update mechanism

### 2.3 Telephony Framework
- [ ] Build NinjaMagic Telephony Service (replaces AOSP TelephonyManager)
- [ ] Expose telephony state via MSI events
- [ ] Agent-accessible telephony API (make calls, send SMS via agent skills)
- [ ] Emergency calling support (E911)
- [ ] Dual-SIM support (Pixel 7 eSIM + physical, Nord N30 dual physical)

**Milestone: Full telephony working — calls, SMS, data on both devices**

---

## Phase 3: HAL & System Services (Weeks 10–18)

### 3.1 Display & Graphics
- [ ] Display HAL for Pixel 7 AMOLED (90Hz, HDR)
- [ ] Display HAL for Nord N30 IPS LCD (120Hz)
- [ ] SurfaceFlinger integration with MSI event hooks
- [ ] GPU HAL: Mali-G710 (Pixel) and Adreno 619 (Nord)
- [ ] Vulkan/OpenGL ES support verified on both

### 3.2 Camera
- [ ] Camera HAL for Pixel 7 (50MP main, 12MP ultrawide, 10.8MP front)
- [ ] Camera HAL for Nord N30 (108MP main, 2MP depth, 2MP macro, 16MP front)
- [ ] Camera events on MSI bus (`camera/capture`, `camera/preview`)
- [ ] Agent-accessible camera (take photos/video via skills)

### 3.3 Sensors
- [ ] Sensor HAL wrapper for accelerometer, gyroscope, compass, proximity, light
- [ ] Sensor data published as MSI events (`sensor/accel`, `sensor/gyro`, etc.)
- [ ] GPS/GNSS HAL (GPS, GLONASS, Galileo, BeiDou)
- [ ] Fingerprint HAL (under-display for Pixel, side-mount for Nord)
- [ ] NFC HAL

### 3.4 Audio
- [ ] Audio HAL for both devices
- [ ] Audio events on MSI bus (`audio/input`, `audio/output`)
- [ ] Agent voice input/output pipeline
- [ ] Bluetooth audio (A2DP, aptX HD)

### 3.5 Power Management
- [ ] Power HAL with MSI energy budget integration
- [ ] Lane-aware power management (throttle low-priority lanes on battery)
- [ ] Charging events (`power/charging`, `power/level`)
- [ ] Biofield-aware power profiles (reduce processing when user state is "resting")

### 3.6 Connectivity
- [ ] WiFi HAL (802.11ac for both, ax for Pixel)
- [ ] Bluetooth HAL (5.2 Pixel, 5.1 Nord) with BLE for wearables
- [ ] USB HAL (USB-C, OTG)
- [ ] NFC HAL

**Milestone: All hardware subsystems accessible via HAL and MSI events**

---

## Phase 4: NinjaMagic Agent (Weeks 14–22)

### 4.1 Agent Core
- [ ] Implement Agent Core as MSI lane (Rust + Kotlin)
- [ ] Context Aggregator: subscribe to all MSI event topics, maintain context window
- [ ] Intent Router: lightweight classifier (TFLite on NPU) for routing to skills
- [ ] Response Generator: on-device LLM integration
- [ ] Conversation manager with MSI AssocStore for memory

### 4.2 On-Device LLM
- [ ] Integrate llama.cpp with Tensor TPU backend (Pixel 7)
- [ ] Integrate llama.cpp/ONNX Runtime with Hexagon DSP backend (Nord N30)
- [ ] Benchmark inference latency: target <2s for first token, >10 tok/s generation
- [ ] Quantize Llama 3.2 3B (Pixel) and 1B (Nord) with tool-calling fine-tune
- [ ] Ship MiniLM embedding model for AssocStore vector operations
- [ ] Cloud fallback API client (OpenAI, Anthropic, user-configurable)

### 4.3 Skill Engine
- [ ] Define Skill interface (SP domain with grants + entrypoint)
- [ ] Built-in Phone Skills:
  - [ ] Make/answer calls
  - [ ] Send/read SMS
  - [ ] Manage contacts
  - [ ] Set alarms/timers
  - [ ] Control media playback
- [ ] Built-in System Skills:
  - [ ] Open/close apps
  - [ ] Change settings (WiFi, Bluetooth, brightness, volume)
  - [ ] File management
  - [ ] Screenshot/screen recording
- [ ] Built-in Web Skills:
  - [ ] Web search
  - [ ] URL fetch and summarize
  - [ ] API calls
- [ ] Custom Skill loader (load .sp files as agent skills at runtime)

### 4.4 MCP Integration
- [ ] MCP Server exposing phone capabilities as tools
- [ ] MCP Client for connecting to external MCP servers
- [ ] Tool registry: auto-discover skills and expose as MCP tools
- [ ] Resource provider: phone state (battery, location, connectivity) as MCP resources

### 4.5 Agent Memory System
- [ ] Working Memory: MSI AssocStore space with TTL-based eviction
- [ ] Episodic Memory: conversation history with vector embeddings
- [ ] Semantic Memory: user preferences, learned patterns (persistent)
- [ ] Procedural Memory: skill execution traces for self-improvement
- [ ] Memory consolidation lane (background, low-priority, runs during charging)

**Milestone: AI agent functional — can answer questions, make calls, control phone via voice/text**

---

## Phase 5: UI Shell & User Experience (Weeks 18–26)

### 5.1 NinjaMagic Launcher
- [ ] Custom home screen with agent-first UI (Jetpack Compose)
- [ ] Persistent agent conversation thread (always accessible)
- [ ] App grid/drawer (Android app compatibility)
- [ ] Notification center with MSI event integration
- [ ] Quick settings panel

### 5.2 Biofield-Aware UI
- [ ] Integrate Space Child Biofield profile
- [ ] UI tempo/animation speed adapts to heart state
- [ ] Color theme shifts based on biofield state (focused=cool, charged=warm)
- [ ] Wearable BLE integration for real-time physiological data
- [ ] Consciousness graph visualization for user identity

### 5.3 System UI
- [ ] Lock screen with agent quick-access
- [ ] Status bar with agent status indicator
- [ ] Volume/power panels
- [ ] Recent apps (with agent context for each)
- [ ] Settings app (with MSI domain/capability management)

### 5.4 Space Child Integration
- [ ] Space Child Auth login flow
- [ ] Profile sync with Space-Child-Dream
- [ ] Artifact crystallization from phone (photos, notes → artifacts)
- [ ] Cross-device agent context sync

**Milestone: Complete phone UX with agent-first interaction model**

---

## Phase 6: Security & Hardening (Weeks 22–28)

### 6.1 Boot Security
- [ ] AVB (Android Verified Boot) signing with custom keys
- [ ] Secure boot chain verification
- [ ] Rollback protection
- [ ] dm-verity for system partitions

### 6.2 MSI Security
- [ ] Domain sealing enforcement in kernel module
- [ ] Hardware attestation via Titan M2 (Pixel) / Qualcomm SPU (Nord)
- [ ] Capability audit logging
- [ ] Agent skill sandboxing verification

### 6.3 Privacy
- [ ] On-device processing audit (verify no unintended data exfiltration)
- [ ] Network traffic monitoring/blocking per MSI domain
- [ ] Encrypted storage for MSI state regions
- [ ] Secure key storage in TEE/SE

### 6.4 Updates
- [ ] OTA update mechanism
- [ ] A/B partition support (both devices)
- [ ] Delta updates for efficiency
- [ ] Agent-managed update flow

**Milestone: Security-hardened OS ready for daily driver use**

---

## Phase 7: Testing & Polish (Weeks 26–32)

### 7.1 Hardware Testing
- [ ] Full telephony regression (calls, SMS, data, VoLTE, emergency)
- [ ] All sensors verified on both devices
- [ ] Camera quality testing
- [ ] Battery life benchmarks (target: >24h with agent active)
- [ ] Thermal testing under sustained agent inference load

### 7.2 Agent Testing
- [ ] Skill coverage tests (all built-in skills)
- [ ] Inference quality benchmarks
- [ ] Memory system stress tests
- [ ] MCP compatibility tests
- [ ] Multi-turn conversation coherence

### 7.3 Integration Testing
- [ ] Android app compatibility (top 50 apps)
- [ ] Space Child ecosystem integration tests
- [ ] Wearable connectivity tests
- [ ] Cross-device sync tests

### 7.4 Release Preparation
- [ ] Flash tool / installer for both devices
- [ ] User documentation
- [ ] Developer documentation (skill development guide)
- [ ] First-run setup wizard
- [ ] Recovery image

**Milestone: v0.1.0 release — installable on Pixel 7 and Nord N30**

---

## Timeline Summary

| Phase | Duration | Weeks | Key Deliverable |
|-------|----------|-------|----------------|
| **Phase 0** | 4 weeks | 1–4 | Build system, both devices boot AOSP |
| **Phase 1** | 6 weeks | 5–10 | MSI kernel module + runtime + SP compiler |
| **Phase 2** | 8 weeks | 7–14 | Full telephony on both devices |
| **Phase 3** | 8 weeks | 10–18 | All HALs and system services |
| **Phase 4** | 8 weeks | 14–22 | AI agent with skills and on-device LLM |
| **Phase 5** | 8 weeks | 18–26 | UI shell with biofield-aware UX |
| **Phase 6** | 6 weeks | 22–28 | Security hardening and verified boot |
| **Phase 7** | 6 weeks | 26–32 | Testing, polish, v0.1.0 release |

**Total estimated timeline: ~32 weeks (8 months)**  
**Note:** Phases overlap. Radio work (Phase 2) starts during MSI development (Phase 1). UI work (Phase 5) starts during agent development (Phase 4).

---

## Build Requirements

### Hardware
- Build server: 16+ cores, 64+ GB RAM, 500+ GB SSD (AOSP is ~300GB)
- Google Pixel 7 (unlocked, non-Verizon)
- OnePlus Nord N30 5G (CPH2513, unlocked)
- USB-C cables for both devices

### Software
- Ubuntu 22.04 or 24.04 LTS (native or WSL2)
- Android SDK Platform Tools (latest)
- AOSP build prerequisites (`make`, `python3`, `git`, `repo`, etc.)
- Rust toolchain (nightly, for MSI runtime)
- Android NDK r26+ (for native HAL/agent code)
- Cross-compilation: `aarch64-linux-gnu-gcc`

### Accounts / Keys
- Google account for AOSP source access
- GitHub repo for ninjamagicOS source
- Test signing keys (generated locally)
- Release AVB signing keys (HSM-backed for production)
