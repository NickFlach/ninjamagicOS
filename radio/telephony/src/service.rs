//! Telephony Service — MSI lane orchestrator for call and SMS managers
//!
//! Runs as an MSI lane within the Telephony domain, subscribing to
//! phone/* events and delegating to CallManager and SmsManager.
//! Publishes structured state updates on telephony/* topics for
//! the NinjaMagic Agent.

use std::sync::Arc;
use std::time::Duration;
use log::{info, warn, error};

use msi::{Substrate, Grant, Perms};
use msi::event::{EventBus, QoS};

use crate::call_manager::CallManager;
use crate::sms_manager::{SmsManager, SmsStatus};

/// The telephony service — bridges RILD events to structured state.
pub struct TelephonyService {
    call_manager: Arc<CallManager>,
    sms_manager: Arc<SmsManager>,
}

impl TelephonyService {
    pub fn new() -> Self {
        TelephonyService {
            call_manager: Arc::new(CallManager::new()),
            sms_manager: Arc::new(SmsManager::new()),
        }
    }

    /// Get a reference to the call manager.
    pub fn calls(&self) -> &CallManager {
        &self.call_manager
    }

    /// Get a reference to the SMS manager.
    pub fn sms(&self) -> &SmsManager {
        &self.sms_manager
    }

    /// Start the telephony service event loop.
    ///
    /// Connects to the MSI substrate, creates a Telephony domain,
    /// subscribes to phone/* events, and dispatches to managers.
    pub fn run(&self, substrate: &Substrate) -> Result<(), String> {
        let event_bus = substrate.event_bus();

        // Create a read-only telephony domain for consuming phone events
        let domain = substrate.domain("TelephonyService")
            .grant(Grant::Events("phone/".into()))
            .grant(Grant::Events("telephony/".into()))
            .grant(Grant::Clock)
            .seal()
            .build()
            .map_err(|e| format!("Domain creation failed: {}", e))?;

        info!("TelephonyService domain created: id={}", domain.id());

        // Subscribe to all phone events from RILD bridge
        let phone_sub = event_bus.subscribe(domain.id(), "phone/")
            .map_err(|e| format!("Subscribe failed: {}", e))?;

        info!("TelephonyService subscribed to phone/* events");

        // Publish service ready
        let _ = event_bus.emit(domain.id(), "telephony/status", "ready");

        // Event processing loop
        loop {
            match phone_sub.wait(Duration::from_millis(500)) {
                Ok(Some(event)) => {
                    let payload = event.payload_str().unwrap_or("");
                    self.dispatch_event(&event.topic, payload, &event_bus, domain.id());
                }
                Ok(None) => {
                    // Timeout — do periodic maintenance
                    self.call_manager.prune_history();
                }
                Err(e) => {
                    error!("Event wait error: {}", e);
                    std::thread::sleep(Duration::from_secs(1));
                }
            }
        }
    }

    /// Dispatch a phone event to the appropriate manager.
    fn dispatch_event(&self, topic: &str, payload: &str,
                      event_bus: &EventBus, domain_id: u32) {
        let json: serde_json::Value = match serde_json::from_str(payload) {
            Ok(v) => v,
            Err(_) => {
                warn!("Invalid JSON payload for topic {}", topic);
                return;
            }
        };

        match topic {
            "phone/call/incoming" => {
                let number = json["number"].as_str().unwrap_or("Unknown");
                let name = json["name"].as_str();
                let slot = json["slot"].as_i64().unwrap_or(0) as i32;

                let call_id = self.call_manager.on_incoming(number, name, slot);

                // Publish structured event for agent
                let agent_payload = serde_json::json!({
                    "call_id": call_id,
                    "number": number,
                    "name": name,
                    "slot": slot,
                    "action_required": true,
                    "suggested_actions": ["answer", "decline", "send_to_voicemail"]
                });
                let _ = event_bus.publish(
                    domain_id,
                    "telephony/call/incoming",
                    agent_payload.to_string().as_bytes(),
                    QoS::AtLeastOnce,
                );
            }

            "phone/call/active" => {
                let call_id = json["call_id"].as_i64().unwrap_or(0) as i32;
                self.call_manager.on_active(call_id);

                let _ = event_bus.emit_json(domain_id, "telephony/call/active", &json);
            }

            "phone/call/ended" => {
                let call_id = json["call_id"].as_i64().unwrap_or(0) as i32;
                let reason = json["reason"].as_i64().unwrap_or(0) as i32;
                self.call_manager.on_ended(call_id, reason);

                // Include duration in the event for the agent
                if let Some(call) = self.call_manager.get_call(call_id) {
                    let duration_secs = call.duration()
                        .map(|d| d.as_secs())
                        .unwrap_or(0);
                    let agent_payload = serde_json::json!({
                        "call_id": call_id,
                        "number": call.number,
                        "duration_secs": duration_secs,
                        "reason": reason,
                    });
                    let _ = event_bus.emit_json(
                        domain_id, "telephony/call/ended", &agent_payload
                    );
                }
            }

            "phone/sms/received" => {
                let from = json["from"].as_str().unwrap_or("Unknown");
                let body = json["body"].as_str().unwrap_or("");
                let ts = json["timestamp"].as_u64().unwrap_or(0);

                self.sms_manager.on_received(from, body, ts);

                // Publish structured event with unread count
                let agent_payload = serde_json::json!({
                    "from": from,
                    "body_preview": &body[..body.len().min(100)],
                    "full_body_len": body.len(),
                    "total_unread": self.sms_manager.total_unread(),
                    "action_required": true,
                    "suggested_actions": ["read", "reply", "dismiss"]
                });
                let _ = event_bus.publish(
                    domain_id,
                    "telephony/sms/received",
                    agent_payload.to_string().as_bytes(),
                    QoS::AtLeastOnce,
                );
            }

            "phone/sms/sent" => {
                let to = json["to"].as_str().unwrap_or("");
                let status = json["status"].as_i64().unwrap_or(0);
                let sms_status = if status == 0 {
                    SmsStatus::Delivered
                } else {
                    SmsStatus::Failed
                };
                self.sms_manager.on_sent(to, "", sms_status);
            }

            "phone/signal/strength" => {
                // Forward signal strength to agent as-is
                let _ = event_bus.emit_json(
                    domain_id, "telephony/signal", &json
                );
            }

            "phone/network/registered" => {
                let _ = event_bus.emit_json(
                    domain_id, "telephony/network", &json
                );
            }

            "phone/sim/state" => {
                let _ = event_bus.emit_json(
                    domain_id, "telephony/sim", &json
                );
            }

            _ => {
                // Forward unknown phone events as-is
                let _ = event_bus.emit(domain_id, topic, payload);
            }
        }
    }
}
