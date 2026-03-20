//! MCP Resource definitions — read-only phone state
//!
//! Resources expose the phone's current state as structured data
//! that external AI agents can read via the MCP protocol.

use serde_json::{json, Value};
use crate::protocol::{Resource, ResourceContents};

/// Return all resources exposed by the NinjaMagic MCP server.
pub fn list_resources() -> Vec<Resource> {
    vec![
        Resource {
            uri: "phone://state/battery".into(),
            name: "Battery Status".into(),
            description: Some("Current battery level, charging state, and estimated time remaining".into()),
            mime_type: Some("application/json".into()),
        },
        Resource {
            uri: "phone://state/network".into(),
            name: "Network Status".into(),
            description: Some("Network type, signal strength, operator name, roaming status".into()),
            mime_type: Some("application/json".into()),
        },
        Resource {
            uri: "phone://state/calls".into(),
            name: "Active Calls".into(),
            description: Some("List of currently active, ringing, or held phone calls".into()),
            mime_type: Some("application/json".into()),
        },
        Resource {
            uri: "phone://state/sms/unread".into(),
            name: "Unread SMS".into(),
            description: Some("Count and preview of unread SMS messages".into()),
            mime_type: Some("application/json".into()),
        },
        Resource {
            uri: "phone://state/location".into(),
            name: "Device Location".into(),
            description: Some("Current GPS coordinates, accuracy, and provider".into()),
            mime_type: Some("application/json".into()),
        },
        Resource {
            uri: "phone://state/connectivity".into(),
            name: "Connectivity Status".into(),
            description: Some("WiFi, Bluetooth, cellular, and airplane mode status".into()),
            mime_type: Some("application/json".into()),
        },
        Resource {
            uri: "phone://state/sensors".into(),
            name: "Sensor Readings".into(),
            description: Some("Latest accelerometer, gyroscope, light, and proximity readings".into()),
            mime_type: Some("application/json".into()),
        },
        Resource {
            uri: "phone://state/biofield".into(),
            name: "Biofield State".into(),
            description: Some("User's current biofield state from connected wearable (heart rate, HRV, state classification)".into()),
            mime_type: Some("application/json".into()),
        },
        Resource {
            uri: "phone://state/device".into(),
            name: "Device Info".into(),
            description: Some("Device model, OS version, MSI capabilities, storage, and memory".into()),
            mime_type: Some("application/json".into()),
        },
        Resource {
            uri: "phone://state/agent".into(),
            name: "Agent Status".into(),
            description: Some("NinjaMagic Agent status: model loaded, memory usage, active skills".into()),
            mime_type: Some("application/json".into()),
        },
    ]
}

/// Read a resource by URI and return its contents.
///
/// In production, each resource queries MSI state regions or
/// system services. For the skeleton, we return structured
/// placeholder data.
pub fn read_resource(uri: &str) -> Option<ResourceContents> {
    let json_content = match uri {
        "phone://state/battery" => json!({
            "level": 85,
            "charging": false,
            "charger_type": "none",
            "temperature_celsius": 28.5,
            "health": "good",
            "estimated_hours_remaining": 18.5
        }),

        "phone://state/network" => json!({
            "type": "LTE",
            "signal_rssi": -75,
            "signal_rsrp": -100,
            "signal_rsrq": -8,
            "operator": "T-Mobile",
            "roaming": false,
            "mcc": "310",
            "mnc": "260"
        }),

        "phone://state/calls" => json!({
            "active_calls": [],
            "recent": [],
            "total_active": 0
        }),

        "phone://state/sms/unread" => json!({
            "total_unread": 0,
            "conversations_with_unread": 0,
            "previews": []
        }),

        "phone://state/location" => json!({
            "latitude": 0.0,
            "longitude": 0.0,
            "accuracy_meters": 0.0,
            "altitude_meters": 0.0,
            "provider": "gps",
            "timestamp_ms": 0,
            "available": false
        }),

        "phone://state/connectivity" => json!({
            "wifi": {
                "enabled": true,
                "connected": true,
                "ssid": "HomeNetwork",
                "signal_dbm": -45,
                "frequency_mhz": 5180,
                "ip_address": "192.168.1.100"
            },
            "bluetooth": {
                "enabled": true,
                "connected_devices": 0
            },
            "cellular": {
                "enabled": true,
                "data_connected": true,
                "type": "LTE"
            },
            "airplane_mode": false
        }),

        "phone://state/sensors" => json!({
            "accelerometer": { "x": 0.0, "y": 0.0, "z": 9.81 },
            "gyroscope": { "x": 0.0, "y": 0.0, "z": 0.0 },
            "light_lux": 350.0,
            "proximity_cm": 5.0,
            "compass_degrees": 0.0,
            "barometer_hpa": 1013.25
        }),

        "phone://state/biofield" => json!({
            "connected": false,
            "wearable_type": null,
            "heart_rate_bpm": null,
            "hrv_ms": null,
            "state": "unknown",
            "last_update_ms": 0
        }),

        "phone://state/device" => json!({
            "model": "NinjaMagic Phone",
            "os_version": "ninjamagicOS 0.1.0",
            "msi_version": "1.0.0",
            "soc": "unknown",
            "accel_type": "unknown",
            "ram_total_mb": 0,
            "ram_available_mb": 0,
            "storage_total_gb": 0,
            "storage_available_gb": 0,
            "screen_on": true,
            "uptime_seconds": 0
        }),

        "phone://state/agent" => json!({
            "status": "running",
            "model_loaded": false,
            "model_name": null,
            "inference_latency_ms": null,
            "memory_working_items": 0,
            "memory_episodic_items": 0,
            "active_skills": [],
            "total_interactions": 0,
            "uptime_seconds": 0
        }),

        _ => return None,
    };

    Some(ResourceContents {
        uri: uri.to_string(),
        mime_type: Some("application/json".into()),
        text: Some(serde_json::to_string_pretty(&json_content).unwrap_or_default()),
        blob: None,
    })
}
