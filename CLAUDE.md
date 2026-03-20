# ninjamagicOS Development Guidelines

## Project Overview

ninjamagicOS is an AI-native mobile operating system targeting Pixel 7 (panther) and OnePlus Nord N30 (larry). It integrates MSI cognitive primitives from SingularisPrime, capability-based security from QuantumOS, and the NinjaMagic Agent as a native OS service.

## Architecture Quick Reference

- **Kernel**: Linux 6.1 (Pixel) / 5.4 (Nord) with MSI kernel module
- **MSI Runtime**: Rust userspace library communicating with kernel module via ioctl/netlink
- **Agent**: Rust + Kotlin, runs as MSI lane with on-device LLM inference
- **HAL**: C++/Rust with per-device backends in `hal/tensor/` and `hal/snapdragon/`
- **Radio**: Custom RILD wrapping vendor .so binaries, MSI event integration
- **Skills**: Written in Shinobi.Substrate (SP), compiled to MSI bytecode

## Key Directories

- `kernel/drivers/msi/` — MSI kernel module (C)
- `msi/runtime/` — Userspace MSI runtime (Rust)
- `msi/compiler/` — SP → MSI bytecode compiler (Rust)
- `hal/common/` — Shared HAL interfaces
- `hal/tensor/` — Pixel 7 HAL backends
- `hal/snapdragon/` — Nord N30 HAL backends
- `radio/` — Custom RIL daemon
- `agent/core/` — NinjaMagic Agent core
- `agent/skills/` — Built-in SP skill programs
- `agent/mcp/` — MCP server/client
- `device/google/panther/` — Pixel 7 device tree
- `device/oneplus/larry/` — Nord N30 device tree

## Development Rules

1. **Never commit vendor blobs** — Extract scripts only, blobs in .gitignore
2. **Never commit signing keys** — Keys directory is gitignored
3. **HAL changes must work on both SoCs** — Test on Pixel 7 AND Nord N30
4. **MSI API changes require spec update** — Keep `msi/spec/` in sync with implementation
5. **Agent skills must declare domain grants** — No ambient authority
6. **Kernel module must not panic** — Lane failures must not crash the substrate
7. **Prefer Rust for userspace** — C only for kernel module and HAL where required
8. **All MSI events use topic namespacing** — e.g., `phone/call/*`, `sensor/accel/*`

## Build Commands (planned)

```bash
# Full system build
make ninjamagic_panther-userdebug  # Pixel 7
make ninjamagic_larry-userdebug    # Nord N30

# MSI kernel module only
cd kernel/drivers/msi && make

# MSI runtime only
cd msi/runtime && cargo build --target aarch64-linux-android

# SP compiler only
cd msi/compiler && cargo build

# Flash to device
fastboot flashall  # From build output directory
```

## Testing

```bash
# MSI kernel module tests
make msi-kunit-test

# MSI runtime integration tests
cd msi/runtime && cargo test

# Agent skill tests
cd agent && cargo test

# Full device tests (requires connected device)
make test-device DEVICE=panther
make test-device DEVICE=larry
```

## Reference Documents

- ADR: `docs/adr/001-architecture-decision-record.md`
- Project Plan: `docs/PROJECT_PLAN.md`
- MSI Spec: `../SingularisPrime/msi/spec.md` (canonical)
- QuantumOS: `../QuantumOS/` (capability security reference)
