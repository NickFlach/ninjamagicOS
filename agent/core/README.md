# NinjaMagic Agent Core

The NinjaMagic Agent is the always-on AI assistant that runs natively as an MSI lane within ninjamagicOS. Inspired by [OpenClaw](https://openclaw.ai/), but purpose-built for a phone OS where the agent IS the runtime, not an app.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Agent Core                            │
│  ┌─────────────┐  ┌─────────────┐  ┌────────────────┐  │
│  │   Context    │  │   Intent    │  │   Response     │  │
│  │  Aggregator  │  │   Router    │  │  Generator     │  │
│  │ (MSI Events) │  │ (Classifier)│  │ (On-Device LLM)│  │
│  └──────┬───────┘  └──────┬──────┘  └───────┬────────┘  │
│         │                 │                  │           │
│  ┌──────▼─────────────────▼──────────────────▼────────┐  │
│  │              Conversation Manager                   │  │
│  │         (MSI AssocStore: episodic memory)           │  │
│  └────────────────────────┬───────────────────────────┘  │
│                           │                              │
│  ┌────────────────────────▼───────────────────────────┐  │
│  │                 Skill Dispatcher                    │  │
│  │  Resolves intent → skill, validates domain grants  │  │
│  └────────────────────────┬───────────────────────────┘  │
│                           │                              │
├───────────────────────────▼──────────────────────────────┤
│                    Skill Engine                           │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────────┐  │
│  │  Phone   │ │  System  │ │   Web    │ │  Creative  │  │
│  │  Skills  │ │  Skills  │ │  Skills  │ │  Skills    │  │
│  └──────────┘ └──────────┘ └──────────┘ └────────────┘  │
├──────────────────────────────────────────────────────────┤
│                    Memory System                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────────┐  │
│  │ Working  │ │ Episodic │ │ Semantic │ │ Procedural │  │
│  │ (active) │ │ (history)│ │ (prefs)  │ │ (traces)   │  │
│  └──────────┘ └──────────┘ └──────────┘ └────────────┘  │
└──────────────────────────────────────────────────────────┘
```

## MSI Integration

The agent runs as a set of MSI lanes within a sealed domain:

```sp
domain "NinjaMagicAgent" {
    // Sensor & phone events (read-only)
    grant Events("phone/")
    grant Events("sensor/")
    grant Events("camera/")
    grant Events("audio/")
    grant Events("power/")
    grant Events("system/")

    // Agent-specific events (read-write)
    grant Events("agent/")
    grant Events("ui/agent/")

    // Memory spaces
    grant Assoc("working", rw)
    grant Assoc("episodic", rw)
    grant Assoc("semantic", rw)
    grant Assoc("procedural", rw)

    // Accelerator access
    grant Accel("npu")
    grant Accel("gpu")

    // Time access
    grant Clock

    seal true
}

// Main agent loop — high priority, big cores
lane "AgentCore" in "NinjaMagicAgent"
    policy { priority high, energy balanced, affinity big } {
    context_sub = listen("phone/", "sensor/", "system/")
    user_sub = listen("agent/input/")

    loop {
        // Aggregate context from all subscriptions
        event = await context_sub timeout(100ms)
        if event != null {
            update_context(event)
        }

        // Check for user input
        input = await user_sub timeout(0)
        if input != null {
            intent = classify_intent(input.payload)
            skill = resolve_skill(intent)
            result = dispatch_skill(skill, input.payload)
            emit("agent/output/", result)
        }
    }
}

// Memory consolidation — low priority, runs during idle/charging
lane "MemoryConsolidation" in "NinjaMagicAgent"
    policy { priority low, energy low, affinity little } {
    power_sub = listen("power/charging")

    loop {
        event = await power_sub
        if event.payload.charging == true {
            consolidate_working_to_episodic()
            consolidate_episodic_to_semantic()
            prune_stale_procedural()
        }
        sleep(300000)  // Check every 5 minutes
    }
}
```

## On-Device LLM

### Pixel 7 (Tensor GS201)
- **Primary**: Llama 3.2 3B Q4_K_M via llama.cpp with Tensor TPU backend
- **Embedding**: all-MiniLM-L6-v2 for AssocStore vectors
- **Classifier**: DistilBERT-tiny for intent routing (TFLite on TPU)

### Nord N30 (Snapdragon 695)
- **Primary**: Llama 3.2 1B Q4_K_M via llama.cpp with Hexagon DSP + Adreno GPU
- **Embedding**: all-MiniLM-L6-v2 (CPU fallback if DSP busy)
- **Classifier**: DistilBERT-tiny for intent routing (TFLite on Hexagon)

### Performance Targets
- Intent classification: <100ms
- First token latency: <2s
- Token generation: >10 tok/s (Pixel), >5 tok/s (Nord)
- Embedding generation: <50ms per sentence

## Skill Interface

Skills are SP programs with declared domain grants:

```sp
// Example: Phone Call Skill
domain "Skill.PhoneCall" {
    grant Events("phone/call/")
    grant Events("agent/skill/phone_call/")
    grant State("contacts", r)
    seal true
}

lane "PhoneCall" in "Skill.PhoneCall"
    policy { priority high, energy balanced } {
    sub = listen("agent/skill/phone_call/invoke")

    loop {
        request = await sub
        number = extract_phone_number(request.payload)
        emit("phone/call/dial", number)
        emit("agent/skill/phone_call/result", "Calling " + number)
    }
}
```

## Technology Stack

- **Core Runtime**: Rust (with Kotlin JNI bridge for Android framework access)
- **LLM Inference**: llama.cpp (C++) with platform-specific backends
- **Embedding**: ONNX Runtime or TFLite
- **MCP**: Rust implementation of Model Context Protocol
- **Skills**: Shinobi.Substrate (SP) programs compiled to MSI bytecode
