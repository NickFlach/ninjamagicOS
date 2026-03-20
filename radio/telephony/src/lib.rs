//! NinjaMagic Telephony Service
//!
//! High-level telephony management running as MSI lanes.
//! Subscribes to phone/* events from the RILD MSI bridge and
//! provides structured call/SMS state to the NinjaMagic Agent.

pub mod call_manager;
pub mod sms_manager;
pub mod service;
