//! Skill Engine — manages and dispatches agent skills
//!
//! Skills are discrete capabilities the agent can execute.
//! Built-in skills are compiled into the agent binary.
//! Custom skills are loaded as SP (Shinobi.Substrate) programs at runtime.

use std::collections::HashMap;
use log::info;

use crate::intent::{Intent, IntentCategory};

/// Result of skill execution.
pub type SkillResult = Result<String, String>;

/// A skill handler function.
type SkillHandler = Box<dyn Fn(&str) -> SkillResult + Send + Sync>;

/// Registry of all available agent skills.
pub struct SkillRegistry {
    skills: HashMap<String, SkillEntry>,
    intent_map: HashMap<IntentCategory, Vec<String>>,
}

struct SkillEntry {
    name: String,
    description: String,
    handler: SkillHandler,
}

impl SkillRegistry {
    pub fn new() -> Self {
        SkillRegistry {
            skills: HashMap::new(),
            intent_map: HashMap::new(),
        }
    }

    /// Register a skill with its handler.
    pub fn register(
        &mut self,
        name: &str,
        description: &str,
        categories: &[IntentCategory],
        handler: impl Fn(&str) -> SkillResult + Send + Sync + 'static,
    ) {
        self.skills.insert(name.to_string(), SkillEntry {
            name: name.to_string(),
            description: description.to_string(),
            handler: Box::new(handler),
        });

        for cat in categories {
            self.intent_map
                .entry(*cat)
                .or_insert_with(Vec::new)
                .push(name.to_string());
        }
    }

    /// Register all built-in skills.
    pub fn register_builtins(&mut self) {
        // Phone call skill
        self.register(
            "phone.call",
            "Make, answer, or end phone calls",
            &[IntentCategory::PhoneCall],
            |input| {
                // In production: dispatches to MSI event bus → RILD
                Ok(format!("Initiating call for: {}", input))
            },
        );

        // SMS skill
        self.register(
            "phone.sms",
            "Send and read text messages",
            &[IntentCategory::Sms],
            |input| {
                Ok(format!("Processing SMS request: {}", input))
            },
        );

        // Settings skill
        self.register(
            "system.settings",
            "Control phone settings (WiFi, Bluetooth, brightness, volume)",
            &[IntentCategory::Settings],
            |input| {
                let lower = input.to_lowercase();
                if lower.contains("wifi") {
                    Ok("Toggling WiFi".to_string())
                } else if lower.contains("bluetooth") {
                    Ok("Toggling Bluetooth".to_string())
                } else if lower.contains("brightness") {
                    Ok("Adjusting brightness".to_string())
                } else if lower.contains("volume") {
                    Ok("Adjusting volume".to_string())
                } else {
                    Ok("Opening settings".to_string())
                }
            },
        );

        // Camera skill
        self.register(
            "camera.capture",
            "Take photos, screenshots, or scan codes",
            &[IntentCategory::Camera],
            |input| {
                let lower = input.to_lowercase();
                if lower.contains("screenshot") {
                    Ok("Taking screenshot".to_string())
                } else {
                    Ok("Opening camera".to_string())
                }
            },
        );

        // Alarm/timer skill
        self.register(
            "system.alarm",
            "Set alarms, timers, and reminders",
            &[IntentCategory::Alarm],
            |input| {
                Ok(format!("Setting alarm/timer: {}", input))
            },
        );

        // App launcher skill
        self.register(
            "system.apps",
            "Open and manage applications",
            &[IntentCategory::Apps],
            |input| {
                Ok(format!("Opening app: {}", input))
            },
        );

        // Media control skill
        self.register(
            "media.control",
            "Play, pause, skip music and media",
            &[IntentCategory::Media],
            |input| {
                Ok(format!("Media control: {}", input))
            },
        );

        // Web search skill
        self.register(
            "web.search",
            "Search the web and fetch information",
            &[IntentCategory::Web],
            |input| {
                Ok(format!("Searching: {}", input))
            },
        );

        // Contacts skill
        self.register(
            "phone.contacts",
            "Search and manage contacts",
            &[IntentCategory::Contacts],
            |input| {
                Ok(format!("Searching contacts: {}", input))
            },
        );

        info!("Registered {} built-in skills", self.skills.len());
    }

    /// Resolve an intent to a skill name.
    pub fn resolve(&self, intent: &Intent) -> Option<&str> {
        if intent.confidence < 0.5 {
            return None;
        }

        self.intent_map
            .get(&intent.category)
            .and_then(|skills| skills.first())
            .map(|s| s.as_str())
    }

    /// Execute a skill by name.
    pub fn execute(&self, skill_name: &str, input: &str) -> SkillResult {
        match self.skills.get(skill_name) {
            Some(entry) => (entry.handler)(input),
            None => Err(format!("Unknown skill: {}", skill_name)),
        }
    }

    /// Get the number of registered skills.
    pub fn count(&self) -> usize {
        self.skills.len()
    }

    /// List all registered skill names and descriptions.
    pub fn list(&self) -> Vec<(&str, &str)> {
        self.skills.values()
            .map(|e| (e.name.as_str(), e.description.as_str()))
            .collect()
    }
}
