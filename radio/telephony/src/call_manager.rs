//! Call Manager — tracks active call state via MSI events
//!
//! Subscribes to phone/call/* events from the RILD bridge and
//! maintains a structured call list. Publishes high-level call
//! state changes for the agent to consume.

use std::collections::HashMap;
use std::time::{Duration, Instant};
use serde::{Serialize, Deserialize};
use parking_lot::RwLock;
use log::{info, warn};

/// Call direction.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum CallDirection {
    Incoming,
    Outgoing,
}

/// Call state machine.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum CallState {
    /// Ringing (incoming) or dialing (outgoing)
    Ringing,
    /// Connected and active
    Active,
    /// On hold
    Held,
    /// Call ended
    Ended,
}

/// A tracked phone call.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Call {
    pub call_id: i32,
    pub number: String,
    pub name: Option<String>,
    pub direction: CallDirection,
    pub state: CallState,
    pub sim_slot: i32,
    pub start_time_ms: u64,
    pub connect_time_ms: Option<u64>,
    pub end_time_ms: Option<u64>,
    pub end_reason: Option<i32>,
}

impl Call {
    /// Get call duration if connected.
    pub fn duration(&self) -> Option<Duration> {
        match (self.connect_time_ms, self.end_time_ms) {
            (Some(start), Some(end)) => Some(Duration::from_millis(end - start)),
            (Some(start), None) => {
                let now = std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)
                    .map(|d| d.as_millis() as u64)
                    .unwrap_or(0);
                Some(Duration::from_millis(now - start))
            }
            _ => None,
        }
    }

    /// Check if the call is currently active (ringing, active, or held).
    pub fn is_live(&self) -> bool {
        matches!(self.state, CallState::Ringing | CallState::Active | CallState::Held)
    }
}

/// Call manager — maintains the active call list.
pub struct CallManager {
    calls: RwLock<HashMap<i32, Call>>,
    next_id: RwLock<i32>,
}

impl CallManager {
    pub fn new() -> Self {
        CallManager {
            calls: RwLock::new(HashMap::new()),
            next_id: RwLock::new(1),
        }
    }

    /// Process an incoming call event from RILD bridge.
    pub fn on_incoming(&self, number: &str, name: Option<&str>, sim_slot: i32) -> i32 {
        let mut next = self.next_id.write();
        let call_id = *next;
        *next += 1;

        let now_ms = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map(|d| d.as_millis() as u64)
            .unwrap_or(0);

        let call = Call {
            call_id,
            number: number.to_string(),
            name: name.map(|s| s.to_string()),
            direction: CallDirection::Incoming,
            state: CallState::Ringing,
            sim_slot,
            start_time_ms: now_ms,
            connect_time_ms: None,
            end_time_ms: None,
            end_reason: None,
        };

        info!("Incoming call: id={} number={} slot={}", call_id, number, sim_slot);
        self.calls.write().insert(call_id, call);
        call_id
    }

    /// Process a call becoming active (answered).
    pub fn on_active(&self, call_id: i32) {
        let now_ms = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map(|d| d.as_millis() as u64)
            .unwrap_or(0);

        let mut calls = self.calls.write();
        if let Some(call) = calls.get_mut(&call_id) {
            call.state = CallState::Active;
            call.connect_time_ms = Some(now_ms);
            info!("Call active: id={} number={}", call_id, call.number);
        } else {
            warn!("on_active for unknown call_id={}", call_id);
        }
    }

    /// Process a call ending.
    pub fn on_ended(&self, call_id: i32, reason: i32) {
        let now_ms = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map(|d| d.as_millis() as u64)
            .unwrap_or(0);

        let mut calls = self.calls.write();
        if let Some(call) = calls.get_mut(&call_id) {
            call.state = CallState::Ended;
            call.end_time_ms = Some(now_ms);
            call.end_reason = Some(reason);

            let duration = call.duration()
                .map(|d| format!("{}s", d.as_secs()))
                .unwrap_or_else(|| "N/A".to_string());
            info!("Call ended: id={} number={} duration={} reason={}",
                  call_id, call.number, duration, reason);
        } else {
            warn!("on_ended for unknown call_id={}", call_id);
        }
    }

    /// Register an outgoing call initiated by the agent.
    pub fn on_outgoing(&self, number: &str, sim_slot: i32) -> i32 {
        let mut next = self.next_id.write();
        let call_id = *next;
        *next += 1;

        let now_ms = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map(|d| d.as_millis() as u64)
            .unwrap_or(0);

        let call = Call {
            call_id,
            number: number.to_string(),
            name: None,
            direction: CallDirection::Outgoing,
            state: CallState::Ringing,
            sim_slot,
            start_time_ms: now_ms,
            connect_time_ms: None,
            end_time_ms: None,
            end_reason: None,
        };

        info!("Outgoing call: id={} number={}", call_id, number);
        self.calls.write().insert(call_id, call);
        call_id
    }

    /// Get a snapshot of all live calls.
    pub fn active_calls(&self) -> Vec<Call> {
        self.calls.read().values()
            .filter(|c| c.is_live())
            .cloned()
            .collect()
    }

    /// Get a specific call by ID.
    pub fn get_call(&self, call_id: i32) -> Option<Call> {
        self.calls.read().get(&call_id).cloned()
    }

    /// Get recent call history (last N ended calls).
    pub fn recent_history(&self, limit: usize) -> Vec<Call> {
        let calls = self.calls.read();
        let mut ended: Vec<Call> = calls.values()
            .filter(|c| c.state == CallState::Ended)
            .cloned()
            .collect();
        ended.sort_by(|a, b| b.end_time_ms.cmp(&a.end_time_ms));
        ended.truncate(limit);
        ended
    }

    /// Clean up old ended calls (keep last 100).
    pub fn prune_history(&self) {
        let mut calls = self.calls.write();
        let mut ended_ids: Vec<(i32, u64)> = calls.iter()
            .filter(|(_, c)| c.state == CallState::Ended)
            .map(|(id, c)| (*id, c.end_time_ms.unwrap_or(0)))
            .collect();

        if ended_ids.len() > 100 {
            ended_ids.sort_by(|a, b| b.1.cmp(&a.1));
            for (id, _) in &ended_ids[100..] {
                calls.remove(id);
            }
        }
    }
}
