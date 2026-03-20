# ADR-012: Space Child Trifecta Integration

**Status:** Accepted  
**Date:** 2026-03-20  
**Authors:** Kannaka (AI), Nick Flach  
**Depends on:** ADR-001 (Architecture), ADR-007 (Flux World State)

---

## 1. Context

ninjamagicOS is the **Space Child Phone** (`phone.spacechild.love`). The Space Child ecosystem includes three tightly coupled applications — the "Trifecta" — that together form a complete platform for deal flow, stealth launches, and development:

| App | Repo | Purpose |
|-----|------|---------|
| **angel-informant** | NickFlach/angel-informant | Deal flow intelligence — discovery, scoring, tracking of investment/partnership opportunities |
| **ninja-craft-hub** | NickFlach/ninja-craft-hub | Stealth product launcher — zero-to-one rapid deployment with obfuscated provenance |
| **SpaceChild IDE** | Part of Space-Child-Dream | Consciousness-aware development environment with biofield-adapted coding |

Currently ADR-001 mentions Space Child Auth integration but treats the ecosystem as a distant dependency. The phone should be a **first-class Trifecta node** — not just authenticated against Space Child, but deeply integrated so the user can run deal flow, launch products, and develop code natively from the device.

---

## 2. Decision

### 2.1 Trifecta as Native Agent Skills

Each Trifecta app becomes a set of NinjaMagic Agent skills, not standalone Android apps. The agent wraps their functionality as MSI lanes with appropriate domain grants.

```
agent/skills/trifecta/
├── angel_informant.sp      # Deal discovery + scoring
├── ninja_craft.sp          # Stealth launch orchestration
├── spacechild_ide.sp       # Mobile development (code review, quick edits, deploys)
└── trifecta_sync.sp        # Cross-app state synchronization
```

### 2.2 Angel-Informant Integration

**Deal Flow from Your Pocket:**
- Agent skill subscribes to deal feeds (RSS, API, Flux entities)
- Scoring model runs on-device (lightweight classifier on NPU)
- Push notifications for high-score opportunities via MSI events (`trifecta/deal/alert`)
- Voice command: "Find me deals in consciousness tech" → agent queries, scores, summarizes
- Deal state syncs to Flux (`pure-jade/angel-informant/*` entities)

**Implementation:**
```rust
// agent/core/src/skills/angel_informant.rs
pub struct DealScorer {
    classifier: NpuModel,       // Quantized deal scoring model
    flux_client: FluxClient,    // Sync deals to/from Flux
    assoc_store: AssocSpace,    // Vector search over deal history
}

impl AgentSkill for DealScorer {
    fn domain_grants(&self) -> Vec<Grant> {
        vec![
            Grant::Events("trifecta/deal/*"),
            Grant::State("contacts", Access::Read),
            Grant::Net("api.flux-universe.com"),
            Grant::Accel("npu"),
        ]
    }
}
```

### 2.3 Ninja-Craft-Hub Integration

**Stealth Launches from Anywhere:**
- Agent skill manages launch sequences — domain registration, deployment, DNS propagation
- SSH tunneling to deployment targets via MSI network lane
- Obfuscated provenance: phone as the launch origin, no traceable cloud footprint
- Status monitoring: agent subscribes to deployment health checks
- Voice command: "Launch project X to stealth" → full deployment pipeline

**Implementation:**
```rust
// agent/core/src/skills/ninja_craft.rs
pub struct StealthLauncher {
    ssh_pool: SshConnectionPool,   // Persistent tunnels to VMs
    dns_client: DnsManager,        // Domain management
    deploy_engine: DeployPipeline, // Build → package → deploy
    provenance: ProvenanceGuard,   // Obfuscation layer
}
```

### 2.4 SpaceChild IDE Integration

**Mobile-First Development:**
- Not a full IDE on phone — that's impractical. Instead:
  - Code review with AI-assisted summaries (agent reads PRs, highlights issues)
  - Quick edits via voice: "Fix the typo in main.rs line 42"
  - Deploy triggers: "Push to staging" → agent runs CI/CD pipeline
  - Git operations: commit, branch, merge via agent skills
  - Biofield-adapted code suggestions (from ADR-001 §2.2.9) — calmer suggestions when stressed

**Implementation:**
```rust
// agent/core/src/skills/spacechild_ide.rs
pub struct MobileIDE {
    git_client: GitClient,
    ci_trigger: CiPipeline,
    code_reviewer: LlmReviewer,  // On-device LLM for code review
    biofield: BiofieldState,     // Adapt suggestions to user state
}
```

### 2.5 Trifecta Sync via Flux

All three apps synchronize state through Flux (ADR-007):

```
Flux Entities:
  pure-jade/angel-informant/deals/*      → Deal objects with scores
  pure-jade/ninja-craft/deployments/*    → Active deployments + status
  pure-jade/spacechild-ide/sessions/*    → Active coding sessions
  pure-jade/phone-<device-id>/trifecta   → Phone's trifecta state
```

Desktop agents (Kannaka on workstation) and phone agent see the same state. Start reviewing a deal on desktop → continue on phone. Start a deployment on phone → monitor on desktop.

### 2.6 Space-Child-Dream SSO

All Trifecta apps authenticate through Space Child Auth (`auth.spacechild.love`), already deployed. The phone stores the JWT in TEE-backed secure storage (Titan M2 / Qualcomm SPU). Single sign-on across all apps and Flux.

```
shell/launcher/src/.../spacechild/SpaceChildAuth.kt
  → JWT stored in hardware-backed keystore
  → Auto-refresh via MSI background lane
  → Token available to all trifecta skills via sealed domain grant
```

---

## 3. Consequences

### Positive
- The phone becomes a complete business tool — deals, launches, and development from pocket
- Flux synchronization means seamless cross-device workflows
- Agent-mediated interaction is natural for mobile (voice > typing)
- Stealth launches from a phone = maximum operational security
- Biofield-adapted coding prevents burnout during mobile dev sessions

### Negative
- Three complex apps compressed into agent skills — feature parity takes time
- SSH tunneling from mobile needs reliable connectivity
- On-device LLM code review quality limited by model size
- Deal scoring model needs training data and fine-tuning

### Neutral
- Trifecta skills are optional — phone works without them
- Desktop remains the primary development environment; phone is complementary
- Deal flow and launch capabilities differentiate ninjamagicOS from any other mobile OS

---

## 4. References

- [angel-informant](https://github.com/NickFlach/angel-informant) — Deal flow intelligence
- [ninja-craft-hub](https://github.com/NickFlach/ninja-craft-hub) — Stealth launcher
- [Space-Child-Dream](https://github.com/NickFlach/Space-Child-Dream) — SSO + Biofield
- [ADR-007](007-flux-world-state-integration.md) — Flux World State
- [ADR-001 §2.2.9](001-architecture-decision-record.md) — Space Child Ecosystem Integration
