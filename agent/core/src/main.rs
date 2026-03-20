//! NinjaMagic Agent — Entry Point
//!
//! Launches the agent as a set of MSI lanes within a sealed domain.
//! This binary is started by ninjamagicOS init during boot (Phase 3
//! of the MSI boot sequence: Cognitive Bootstrap).

use std::time::Duration;
use log::{info, error, warn};

use msi::{Substrate, Grant, Perms};
use msi::lane::{Lane, LanePolicy, Priority, EnergyBudget, Affinity};

mod context;
mod intent;
mod skill;
mod memory;
mod inference;
mod config;

fn main() {
    // Initialize logging
    #[cfg(target_os = "android")]
    android_logger::init_once(
        android_logger::Config::default()
            .with_max_level(log::LevelFilter::Info)
            .with_tag("NinjaMagicAgent"),
    );

    #[cfg(not(target_os = "android"))]
    env_logger::init();

    info!("=== NinjaMagic Agent v0.1.0 starting ===");

    // Phase 0: Connect to MSI substrate
    let substrate = match Substrate::connect() {
        Ok(s) => {
            info!("MSI substrate connected — v{}", s.version());
            s
        }
        Err(e) => {
            error!("Failed to connect to MSI substrate: {}", e);
            error!("Is the MSI kernel module loaded?");
            std::process::exit(1);
        }
    };

    // Log hardware capabilities
    let caps = substrate.capabilities();
    info!("Hardware: npu={} gpu={} dsp={} security={:?}",
          caps.accel_npu, caps.accel_gpu, caps.accel_dsp,
          caps.security_model);

    // Phase 1: Create the agent domain with capability grants
    let domain = match substrate.domain("NinjaMagicAgent")
        // Phone events (read from telephony subsystem)
        .grant(Grant::Events("phone/".into()))
        // Sensor events
        .grant(Grant::Events("sensor/".into()))
        // Camera events
        .grant(Grant::Events("camera/".into()))
        // Audio events
        .grant(Grant::Events("audio/".into()))
        // Power/battery events
        .grant(Grant::Events("power/".into()))
        // System events
        .grant(Grant::Events("system/".into()))
        // Agent's own events (bidirectional)
        .grant(Grant::Events("agent/".into()))
        // UI events for agent responses
        .grant(Grant::Events("ui/agent/".into()))
        // Memory spaces
        .grant(Grant::Assoc("working".into(), Perms::ReadWrite))
        .grant(Grant::Assoc("episodic".into(), Perms::ReadWrite))
        .grant(Grant::Assoc("semantic".into(), Perms::ReadWrite))
        .grant(Grant::Assoc("procedural".into(), Perms::ReadWrite))
        // Accelerator access for LLM inference
        .grant(Grant::Accel(if caps.accel_npu { "npu" } else { "gpu" }.into()))
        .grant(Grant::Clock)
        // Seal — no further grants allowed
        .seal()
        .build()
    {
        Ok(d) => {
            info!("Agent domain created — id={} sealed={}", d.id(), d.is_sealed());
            d
        }
        Err(e) => {
            error!("Failed to create agent domain: {}", e);
            std::process::exit(1);
        }
    };

    // Phase 2: Initialize memory spaces
    info!("Initializing agent memory spaces");
    let working_memory = msi::AssocStore::new("working");
    let episodic_memory = msi::AssocStore::new("episodic");
    let semantic_memory = msi::AssocStore::new("semantic");
    let _procedural_memory = msi::AssocStore::new("procedural");

    // Phase 3: Initialize inference engine
    let device_profile = if caps.accel_npu {
        info!("NPU detected — using Tensor TPU inference profile (3B model)");
        inference::DeviceProfile::TensorTPU
    } else if caps.accel_dsp {
        info!("DSP detected — using Hexagon inference profile (1B model)");
        inference::DeviceProfile::HexagonDSP
    } else {
        warn!("No accelerator detected — using CPU fallback");
        inference::DeviceProfile::CpuFallback
    };

    let _inference = inference::InferenceEngine::new(device_profile);

    // Phase 4: Register built-in skills
    let mut skill_registry = skill::SkillRegistry::new();
    skill_registry.register_builtins();
    info!("Registered {} built-in skills", skill_registry.count());

    // Phase 5: Start the agent core loop
    info!("Starting agent core loop");

    let event_bus = substrate.event_bus();

    // Subscribe to all relevant event topics
    let context_sub = match event_bus.subscribe(domain.id(), "phone/") {
        Ok(s) => s,
        Err(e) => {
            error!("Failed to subscribe to phone events: {}", e);
            std::process::exit(1);
        }
    };

    let agent_input_sub = match event_bus.subscribe(domain.id(), "agent/input/") {
        Ok(s) => s,
        Err(e) => {
            error!("Failed to subscribe to agent input: {}", e);
            std::process::exit(1);
        }
    };

    // Publish agent ready event
    let _ = event_bus.emit(domain.id(), "agent/status", "ready");
    info!("=== NinjaMagic Agent ready ===");

    // Main cognitive loop
    loop {
        // 1. Aggregate context from phone events
        if let Ok(Some(event)) = context_sub.wait(Duration::from_millis(100)) {
            // Update working memory with latest context
            let ctx_value = msi::AssocValue::from_str(
                &format!("{}:{}", event.topic, event.payload_str().unwrap_or(""))
            ).with_meta("topic", &event.topic);

            let _ = working_memory.put(
                &format!("ctx:{}", event.id),
                ctx_value,
            );

            // Evict old context (keep last 100 events)
            if working_memory.len() > 100 {
                let _ = working_memory.forget_older_than(
                    Duration::from_secs(300).as_nanos() as u64
                );
            }
        }

        // 2. Check for user input
        if let Ok(Some(input)) = agent_input_sub.poll() {
            info!("User input received: topic={}", input.topic);

            let input_text = input.payload_str().unwrap_or("").to_string();

            // Classify intent
            let intent = intent::classify(&input_text);
            info!("Intent classified: {:?}", intent);

            // Route to skill
            if let Some(skill_name) = skill_registry.resolve(&intent) {
                info!("Dispatching to skill: {}", skill_name);

                // Execute skill
                match skill_registry.execute(skill_name, &input_text) {
                    Ok(result) => {
                        // Store in episodic memory
                        let episode = msi::AssocValue::from_str(&format!(
                            "{{\"input\":\"{}\",\"intent\":\"{:?}\",\"skill\":\"{}\",\"result\":\"{}\"}}",
                            input_text, intent, skill_name, result
                        ));
                        let _ = episodic_memory.put(
                            &format!("ep:{}", input.id),
                            episode,
                        );

                        // Publish response
                        let _ = event_bus.emit(
                            domain.id(),
                            "agent/output/response",
                            &result,
                        );
                    }
                    Err(e) => {
                        warn!("Skill execution failed: {}", e);
                        let _ = event_bus.emit(
                            domain.id(),
                            "agent/output/error",
                            &format!("Skill error: {}", e),
                        );
                    }
                }
            } else {
                // No skill matched — use LLM for general response
                info!("No skill matched — generating LLM response");
                let _ = event_bus.emit(
                    domain.id(),
                    "agent/output/response",
                    "I'm still learning new skills. Let me think about that...",
                );
            }
        }
    }
}
