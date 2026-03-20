# ninjamagicOS Developer Guide

**Version 0.1.0** — Build skills, extend the agent, hack the substrate.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────┐
│                   UI Shell                       │
│  NinjaMagic Launcher (Jetpack Compose)          │
│  Home │ Agent Chat │ App Drawer │ System UI     │
├─────────────────────────────────────────────────┤
│              NinjaMagic Agent                    │
│  Intent │ Skills │ Memory │ Inference │ MCP     │
├─────────────────────────────────────────────────┤
│              MSI Runtime (Rust)                  │
│  Domains │ Lanes │ Events │ State │ Assoc       │
├─────────────────────────────────────────────────┤
│           MSI Kernel Module (C)                  │
│  /dev/msi │ ioctl │ domain │ lane │ event       │
├─────────────────────────────────────────────────┤
│     Hardware Abstraction Layer (HAL)            │
│  Radio │ NPU (TPU/Hexagon) │ Sensors │ Camera  │
├─────────────────────────────────────────────────┤
│     Security Layer                              │
│  AVB │ MSI Security │ Privacy Guard │ OTA       │
└─────────────────────────────────────────────────┘
```

---

## Repository Structure

```
ninjamagicOS/
├── kernel/drivers/msi/     # MSI kernel module (C)
│   ├── msi_core.c          # Module init, /dev/msi char device
│   ├── msi_domain.c        # Capability domain management
│   ├── msi_lane.c          # Lane (execution context) scheduling
│   ├── msi_event.c         # Event pub/sub bus
│   ├── msi_state.c         # Addressable state regions
│   └── msi_ioctl.c         # Userspace ioctl dispatch
│
├── msi/
│   ├── runtime/            # MSI userspace runtime (Rust)
│   │   ├── src/lib.rs      # Crate root
│   │   ├── src/substrate.rs # Substrate connection handle
│   │   ├── src/domain.rs   # Domain builder and handle
│   │   ├── src/lane.rs     # Lane abstraction
│   │   ├── src/event.rs    # Event bus client
│   │   ├── src/state.rs    # State region access
│   │   ├── src/assoc.rs    # Associative memory store
│   │   └── src/ffi.rs      # Kernel ioctl FFI bindings
│   └── compiler/           # SP (Shinobi.Substrate) compiler
│       ├── src/lexer.rs    # Tokenizer
│       ├── src/parser.rs   # Recursive descent parser
│       ├── src/ast.rs      # Abstract syntax tree
│       └── src/lowering.rs # SP → Rust codegen
│
├── agent/
│   ├── core/               # NinjaMagic Agent (Rust)
│   │   ├── src/main.rs     # Agent entry point and cognitive loop
│   │   ├── src/context.rs  # Context aggregator (phone state)
│   │   ├── src/intent.rs   # Intent classifier
│   │   ├── src/skill.rs    # Skill engine and registry
│   │   ├── src/memory.rs   # 4-tier memory system
│   │   ├── src/inference.rs # On-device LLM inference
│   │   └── src/config.rs   # Runtime configuration
│   └── mcp/                # MCP server (Rust)
│       ├── src/protocol.rs # JSON-RPC 2.0 types
│       ├── src/tools.rs    # 17 phone capability tools
│       ├── src/resources.rs # 10 phone state resources
│       ├── src/server.rs   # Request dispatcher
│       └── src/transport.rs # Stdio transport
│
├── radio/
│   ├── rild/               # Custom RILD (C++)
│   │   ├── main.cpp        # RILD entry point
│   │   ├── msi_bridge.cpp  # MSI event bridge
│   │   └── vendor_ril.cpp  # Dynamic vendor RIL loader
│   └── telephony/          # Telephony service (Rust)
│       ├── src/call_manager.rs
│       ├── src/sms_manager.rs
│       └── src/service.rs
│
├── hal/
│   ├── common/             # Unified HAL interfaces
│   │   ├── INinjaMagicRadio.h
│   │   └── INinjaMagicNPU.h
│   ├── tensor/npu/         # Google Tensor TPU backend
│   └── snapdragon/npu/     # Qualcomm Hexagon DSP backend
│
├── shell/launcher/         # NinjaMagic Launcher (Kotlin/Compose)
│   ├── src/main/java/com/ninjamagic/launcher/
│   │   ├── ui/home/        # Home screen
│   │   ├── ui/agent/       # Agent chat
│   │   ├── ui/apps/        # App drawer
│   │   ├── ui/systemui/    # Status bar, lock screen
│   │   ├── ui/setup/       # First-run wizard
│   │   ├── biofield/       # Biofield service and state
│   │   └── spacechild/     # Space Child auth
│   └── build.gradle.kts
│
├── security/
│   ├── avb/                # Verified boot
│   ├── msi/                # MSI domain security
│   ├── privacy/            # Privacy guard
│   └── ota/                # OTA update manager
│
├── device/
│   ├── google/panther/     # Pixel 7 device config
│   └── oneplus/larry/      # Nord N30 device config
│
├── tests/
│   ├── hardware/           # Telephony, sensor tests
│   ├── agent/              # Agent, MCP tests
│   └── integration/        # App compat, ecosystem tests
│
├── tools/flash/            # Flash tool
├── examples/               # Example .sp programs
└── docs/                   # Documentation
```

---

## Building

### Prerequisites
- Ubuntu 22.04+ (native or WSL2)
- Rust nightly toolchain
- Android NDK r26+
- AOSP build prerequisites

### Build Commands

```bash
# Clone
git clone https://github.com/NickFlach/ninjamagicOS.git
cd ninjamagicOS

# Build Rust crates
cargo build --release

# Build for specific device (requires AOSP tree)
source build/envsetup.sh
lunch ninjamagic_panther-userdebug   # Pixel 7
# or
lunch ninjamagic_larry-userdebug     # Nord N30

make -j$(nproc)
```

---

## Developing Agent Skills

### Skill Architecture

Skills are the agent's capabilities. Each skill handles a specific intent category and executes actions via MSI events.

```rust
// agent/core/src/skill.rs

pub struct Skill {
    pub name: &'static str,
    pub description: &'static str,
    pub intents: Vec<IntentCategory>,
    pub required_grants: Vec<MsiGrant>,
}
```

### Built-in Skills

| Skill | Intent | MSI Events |
|-------|--------|-----------|
| `phone_call` | Call | `rild/command/dial`, `rild/command/answer` |
| `sms_send` | Text | `rild/command/sms_send` |
| `settings_wifi` | Settings | `system/settings/wifi` |
| `camera_capture` | Camera | `system/camera/capture` |
| `alarm_set` | Alarm | `system/alarm/set` |
| `app_launch` | AppLaunch | `system/app/launch` |
| `media_control` | Media | `system/media/control` |
| `web_search` | WebSearch | `system/web/search` |
| `contacts_lookup` | Contacts | `system/contacts/query` |

### Writing a Custom Skill (SP)

Use the Shinobi.Substrate language to write cognitive programs:

```sp
domain weather_skill {
    grant events rw "agent/skill/"
    grant events r "system/location/"
    grant state rw "weather_cache"
    grant accel r "npu"

    lane fetch_weather(priority: normal, energy: balanced) {
        let location = event.wait("system/location/update")
        let forecast = web.fetch("https://api.weather.gov/...")
        state.write("weather_cache", forecast)
        event.publish("agent/skill/weather/result", forecast)
    }
}
```

Compile with `spc`:
```bash
cargo run --bin spc -- examples/weather_skill.sp -o weather_skill.rs
```

### MCP Tool Development

To expose a new capability via MCP, add to `agent/mcp/src/tools.rs`:

```rust
McpTool {
    name: "weather_check",
    description: "Check weather forecast for a location",
    input_schema: json!({
        "type": "object",
        "properties": {
            "location": { "type": "string", "description": "City or coordinates" }
        },
        "required": ["location"]
    }),
}
```

Then implement the handler in `execute_tool()`.

---

## MSI Programming

### Core Concepts

- **Domain** — Capability container. Has grants (permissions). Once sealed, immutable.
- **Lane** — Execution context (like a thread, but with scheduling policy).
- **Event** — Topic-based pub/sub message. Topics are hierarchical (`system/battery/level`).
- **State** — Addressable byte buffer mapped into a domain.
- **Assoc** — Associative memory store for vector similarity queries.

### Rust API

```rust
use msi_runtime::*;

// Connect to MSI substrate
let substrate = Substrate::connect()?;

// Create a domain with grants
let domain = substrate.domain("my_skill")
    .grant(Grant::Events { topic: "system/", perm: Permission::Read })
    .grant(Grant::State { name: "cache", perm: Permission::ReadWrite })
    .seal()?;

// Spawn a lane
let lane = domain.spawn_lane("worker", Priority::Normal, Energy::Balanced)?;

// Subscribe to events
let sub = domain.event_bus().subscribe("system/battery/level")?;

// Wait for event
let event = sub.wait()?;
println!("Battery: {}", String::from_utf8_lossy(&event.payload));

// Associative memory
let assoc = domain.assoc_store("memory")?;
assoc.put("greeting", b"Hello!", &[0.1, 0.9, 0.3, 0.7])?;
let results = assoc.query(&[0.1, 0.85, 0.35, 0.65], 5)?;
```

---

## HAL Development

### Adding a New HAL Backend

1. Create header in `hal/common/` with the unified interface
2. Create implementation in `hal/<vendor>/<component>/`
3. Implement all functions from the interface header
4. Add build rules to `Android.mk` or `CMakeLists.txt`

### NPU HAL Interface

```c
// hal/common/INinjaMagicNPU.h

int npu_init(NpuAccelType type, NpuConfig *config);
int npu_load_model(const char *path, NpuModelFormat fmt, NpuPrecision prec, NpuModelHandle *out);
int npu_infer(NpuModelHandle model, const float *input, size_t in_len, float *output, size_t out_len);
int npu_embed(NpuModelHandle model, const char *text, float *embedding, size_t dim);
void npu_shutdown(void);
```

---

## Testing

### Run All Tests

```bash
# Hardware tests (device connected via USB)
./tests/hardware/telephony_test.sh --device $(adb get-serialno)
./tests/hardware/sensor_test.sh

# Agent tests (MCP protocol only, no device needed)
./tests/agent/agent_test.sh --mcp-only

# Agent tests (on-device)
./tests/agent/agent_test.sh --device $(adb get-serialno)

# Integration tests
./tests/integration/integration_test.sh
```

### Test Results

Results are saved to `tests/*/results/` with timestamped log files.

---

## Security Model

### Domain Lifecycle
1. `domainCreate("name")` — creates empty domain
2. `domainGrant(type, perm, resource)` — add capabilities (repeatable)
3. `domainSeal()` — lock down grants permanently
4. Domain operates within its grants
5. Any access outside grants → violation logged, lane killed

### Attestation
Any domain can be attested via the hardware secure element:
```c
msi_attest_domain(domain_id, nonce, &result);
// result.signature is signed by Titan M2 / Qualcomm SPU
```

---

## Contributing

1. Fork the repo
2. Create a feature branch
3. Follow existing code style
4. Add tests for new functionality
5. Submit a pull request

---

## License

ninjamagicOS is proprietary software by NinjaMagic / Space Child.
