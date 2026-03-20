//! NinjaMagic Agent — AI-native phone assistant for ninjamagicOS
//!
//! The agent runs as a set of MSI lanes within a sealed domain,
//! subscribing to all phone subsystem events and dispatching
//! user intents to skills via the skill engine.
//!
//! # Architecture
//!
//! - **AgentCore** lane (high priority, big cores): context aggregation + intent routing
//! - **SkillDispatcher** lane (normal priority): executes matched skills
//! - **MemoryConsolidation** lane (low priority, little cores): background memory management
//! - **InferenceEngine**: on-device LLM for response generation
//! - **SkillRegistry**: manages built-in and custom skills

pub mod context;
pub mod intent;
pub mod skill;
pub mod memory;
pub mod inference;
pub mod config;
