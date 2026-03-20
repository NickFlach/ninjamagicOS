//! Agent configuration — runtime settings and user preferences

use serde::{Serialize, Deserialize};

/// Agent configuration stored in MSI state or on-device config file.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AgentConfig {
    /// Agent name (user-customizable)
    pub name: String,

    /// Voice input enabled
    pub voice_enabled: bool,

    /// Proactive notifications (agent reaches out without user prompt)
    pub proactive_enabled: bool,

    /// Cloud inference fallback
    pub cloud_fallback: bool,

    /// Cloud provider config (if cloud_fallback is true)
    pub cloud_provider: Option<String>,
    pub cloud_api_key: Option<String>,
    pub cloud_model: Option<String>,

    /// Memory retention settings
    pub working_memory_ttl_secs: u64,
    pub episodic_memory_max_entries: usize,

    /// Energy budget preference
    pub energy_mode: EnergyMode,

    /// Space Child profile sync enabled
    pub spacechild_sync: bool,

    /// Wearable biofield integration enabled
    pub biofield_enabled: bool,
}

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum EnergyMode {
    /// Minimal agent activity, longest battery life
    PowerSaver,
    /// Balanced agent responsiveness and battery
    Balanced,
    /// Maximum agent performance, shortest battery life
    Performance,
}

impl Default for AgentConfig {
    fn default() -> Self {
        AgentConfig {
            name: "Ninja".to_string(),
            voice_enabled: true,
            proactive_enabled: false,
            cloud_fallback: false,
            cloud_provider: None,
            cloud_api_key: None,
            cloud_model: None,
            working_memory_ttl_secs: 300,
            episodic_memory_max_entries: 10000,
            energy_mode: EnergyMode::Balanced,
            spacechild_sync: true,
            biofield_enabled: false,
        }
    }
}
