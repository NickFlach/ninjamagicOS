//! NinjaMagic MCP Server
//!
//! Implements the Model Context Protocol (MCP) to expose phone
//! capabilities as tools and phone state as resources. This allows
//! external AI agents (Claude, GPT, etc.) to interact with the
//! phone through a standardized protocol.
//!
//! ## Tools (actions the agent can take)
//! - `phone_dial` — Make a phone call
//! - `phone_hangup` — End a call
//! - `phone_answer` — Answer incoming call
//! - `sms_send` — Send an SMS
//! - `sms_read` — Read SMS conversations
//! - `settings_wifi` — Toggle WiFi
//! - `settings_bluetooth` — Toggle Bluetooth
//! - `settings_brightness` — Set screen brightness
//! - `camera_capture` — Take a photo
//! - `app_launch` — Launch an application
//! - `alarm_set` — Set an alarm
//! - `web_search` — Perform a web search
//!
//! ## Resources (read-only phone state)
//! - `phone://state/battery` — Battery level and charging state
//! - `phone://state/network` — Network type, signal, operator
//! - `phone://state/calls` — Active call list
//! - `phone://state/sms/unread` — Unread SMS count and previews
//! - `phone://state/location` — Current GPS coordinates
//! - `phone://state/connectivity` — WiFi/Bluetooth/cellular status
//! - `phone://state/sensors` — Latest sensor readings
//! - `phone://state/biofield` — User biofield state (if wearable connected)

pub mod protocol;
pub mod tools;
pub mod resources;
pub mod server;
pub mod transport;
