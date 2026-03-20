//! Context Aggregator — maintains a rolling window of phone state
//!
//! Subscribes to all MSI event topics and maintains a structured
//! representation of the phone's current state for the agent to
//! reference when processing user requests.

use serde::{Serialize, Deserialize};
use std::collections::HashMap;
use std::time::{SystemTime, UNIX_EPOCH};
use parking_lot::RwLock;

/// Current phone context — a snapshot of all relevant state.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct PhoneContext {
    /// Active call state
    pub call: Option<CallState>,
    /// Recent SMS messages (last 10)
    pub recent_sms: Vec<SmsMessage>,
    /// Network/signal state
    pub network: NetworkState,
    /// Battery/power state
    pub power: PowerState,
    /// Last known location
    pub location: Option<LocationState>,
    /// Active sensors
    pub sensors: HashMap<String, String>,
    /// Timestamp of last update
    pub last_updated_ns: u64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CallState {
    pub call_id: i32,
    pub number: String,
    pub name: Option<String>,
    pub direction: CallDirection,
    pub duration_secs: i32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum CallDirection {
    Incoming,
    Outgoing,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SmsMessage {
    pub from: String,
    pub body: String,
    pub timestamp: u64,
    pub direction: SmsDirection,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum SmsDirection {
    Received,
    Sent,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct NetworkState {
    pub operator: String,
    pub network_type: String,
    pub roaming: bool,
    pub signal_rssi: i32,
    pub signal_rsrp: i32,
    pub data_connected: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct PowerState {
    pub battery_percent: u8,
    pub charging: bool,
    pub power_save: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LocationState {
    pub latitude: f64,
    pub longitude: f64,
    pub accuracy_m: f32,
    pub timestamp: u64,
}

/// Thread-safe context aggregator.
pub struct ContextAggregator {
    context: RwLock<PhoneContext>,
}

impl ContextAggregator {
    pub fn new() -> Self {
        ContextAggregator {
            context: RwLock::new(PhoneContext::default()),
        }
    }

    /// Get a snapshot of the current phone context.
    pub fn snapshot(&self) -> PhoneContext {
        self.context.read().clone()
    }

    /// Update context from an MSI event.
    pub fn ingest(&self, topic: &str, payload: &str) {
        let mut ctx = self.context.write();
        ctx.last_updated_ns = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_nanos() as u64)
            .unwrap_or(0);

        // Route event to appropriate context field
        if topic.starts_with("phone/call/") {
            self.update_call(&mut ctx, topic, payload);
        } else if topic.starts_with("phone/sms/") {
            self.update_sms(&mut ctx, topic, payload);
        } else if topic.starts_with("phone/signal/") {
            self.update_signal(&mut ctx, payload);
        } else if topic.starts_with("phone/network/") {
            self.update_network(&mut ctx, payload);
        } else if topic.starts_with("phone/data/") {
            self.update_data(&mut ctx, topic, payload);
        } else if topic.starts_with("power/") {
            self.update_power(&mut ctx, payload);
        } else if topic.starts_with("sensor/gps/") {
            self.update_location(&mut ctx, payload);
        } else if topic.starts_with("sensor/") {
            ctx.sensors.insert(topic.to_string(), payload.to_string());
        }
    }

    fn update_call(&self, ctx: &mut PhoneContext, topic: &str, payload: &str) {
        if let Ok(v) = serde_json::from_str::<serde_json::Value>(payload) {
            match topic {
                "phone/call/incoming" => {
                    ctx.call = Some(CallState {
                        call_id: 0,
                        number: v["number"].as_str().unwrap_or("").to_string(),
                        name: v["name"].as_str().map(|s| s.to_string()),
                        direction: CallDirection::Incoming,
                        duration_secs: 0,
                    });
                }
                "phone/call/active" => {
                    if let Some(ref mut call) = ctx.call {
                        call.call_id = v["call_id"].as_i64().unwrap_or(0) as i32;
                    }
                }
                "phone/call/ended" => {
                    ctx.call = None;
                }
                _ => {}
            }
        }
    }

    fn update_sms(&self, ctx: &mut PhoneContext, topic: &str, payload: &str) {
        if let Ok(v) = serde_json::from_str::<serde_json::Value>(payload) {
            if topic == "phone/sms/received" {
                ctx.recent_sms.push(SmsMessage {
                    from: v["from"].as_str().unwrap_or("").to_string(),
                    body: v["body"].as_str().unwrap_or("").to_string(),
                    timestamp: v["timestamp"].as_u64().unwrap_or(0),
                    direction: SmsDirection::Received,
                });
                if ctx.recent_sms.len() > 10 {
                    ctx.recent_sms.remove(0);
                }
            }
        }
    }

    fn update_signal(&self, ctx: &mut PhoneContext, payload: &str) {
        if let Ok(v) = serde_json::from_str::<serde_json::Value>(payload) {
            ctx.network.signal_rssi = v["rssi"].as_i64().unwrap_or(0) as i32;
            ctx.network.signal_rsrp = v["rsrp"].as_i64().unwrap_or(0) as i32;
        }
    }

    fn update_network(&self, ctx: &mut PhoneContext, payload: &str) {
        if let Ok(v) = serde_json::from_str::<serde_json::Value>(payload) {
            ctx.network.operator = v["operator"].as_str().unwrap_or("").to_string();
            ctx.network.network_type = v["type"].as_str().unwrap_or("").to_string();
            ctx.network.roaming = v["roaming"].as_bool().unwrap_or(false);
        }
    }

    fn update_data(&self, ctx: &mut PhoneContext, topic: &str, _payload: &str) {
        ctx.network.data_connected = topic == "phone/data/connected";
    }

    fn update_power(&self, ctx: &mut PhoneContext, payload: &str) {
        if let Ok(v) = serde_json::from_str::<serde_json::Value>(payload) {
            if let Some(pct) = v["percent"].as_u64() {
                ctx.power.battery_percent = pct as u8;
            }
            if let Some(charging) = v["charging"].as_bool() {
                ctx.power.charging = charging;
            }
        }
    }

    fn update_location(&self, ctx: &mut PhoneContext, payload: &str) {
        if let Ok(v) = serde_json::from_str::<serde_json::Value>(payload) {
            ctx.location = Some(LocationState {
                latitude: v["lat"].as_f64().unwrap_or(0.0),
                longitude: v["lon"].as_f64().unwrap_or(0.0),
                accuracy_m: v["accuracy"].as_f64().unwrap_or(0.0) as f32,
                timestamp: v["timestamp"].as_u64().unwrap_or(0),
            });
        }
    }
}
