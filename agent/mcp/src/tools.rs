//! MCP Tool definitions and execution handlers
//!
//! Each tool maps to a phone capability that external AI agents
//! can invoke through the MCP protocol.

use serde_json::{json, Value};
use crate::protocol::{Tool, ToolResult};

/// Return all tools exposed by the NinjaMagic MCP server.
pub fn list_tools() -> Vec<Tool> {
    vec![
        Tool {
            name: "phone_dial".into(),
            description: "Make a phone call to the specified number".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "number": { "type": "string", "description": "Phone number to call" },
                    "sim_slot": { "type": "integer", "description": "SIM slot (0 or 1)", "default": 0 }
                },
                "required": ["number"]
            }),
        },
        Tool {
            name: "phone_answer".into(),
            description: "Answer an incoming phone call".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "call_id": { "type": "integer", "description": "Call ID to answer (from active calls)" }
                },
                "required": ["call_id"]
            }),
        },
        Tool {
            name: "phone_hangup".into(),
            description: "End a phone call".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "call_id": { "type": "integer", "description": "Call ID to hang up" }
                },
                "required": ["call_id"]
            }),
        },
        Tool {
            name: "sms_send".into(),
            description: "Send an SMS text message".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "to": { "type": "string", "description": "Recipient phone number" },
                    "body": { "type": "string", "description": "Message text" },
                    "sim_slot": { "type": "integer", "description": "SIM slot (0 or 1)", "default": 0 }
                },
                "required": ["to", "body"]
            }),
        },
        Tool {
            name: "sms_read".into(),
            description: "Read SMS conversations. Returns recent messages from a contact or all unread messages.".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "contact": { "type": "string", "description": "Phone number to read conversation with (optional)" },
                    "unread_only": { "type": "boolean", "description": "Only return unread messages", "default": false },
                    "limit": { "type": "integer", "description": "Max messages to return", "default": 10 }
                }
            }),
        },
        Tool {
            name: "sms_search".into(),
            description: "Search SMS messages by text content".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "query": { "type": "string", "description": "Search text" },
                    "limit": { "type": "integer", "description": "Max results", "default": 20 }
                },
                "required": ["query"]
            }),
        },
        Tool {
            name: "settings_wifi".into(),
            description: "Toggle WiFi on or off".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "enabled": { "type": "boolean", "description": "true to enable, false to disable" }
                },
                "required": ["enabled"]
            }),
        },
        Tool {
            name: "settings_bluetooth".into(),
            description: "Toggle Bluetooth on or off".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "enabled": { "type": "boolean", "description": "true to enable, false to disable" }
                },
                "required": ["enabled"]
            }),
        },
        Tool {
            name: "settings_brightness".into(),
            description: "Set screen brightness level".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "level": { "type": "integer", "description": "Brightness 0-255", "minimum": 0, "maximum": 255 },
                    "auto": { "type": "boolean", "description": "Enable auto-brightness", "default": false }
                },
                "required": ["level"]
            }),
        },
        Tool {
            name: "settings_volume".into(),
            description: "Set volume level for a stream".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "stream": { "type": "string", "enum": ["ring", "media", "alarm", "notification"], "description": "Audio stream" },
                    "level": { "type": "integer", "description": "Volume 0-15", "minimum": 0, "maximum": 15 }
                },
                "required": ["stream", "level"]
            }),
        },
        Tool {
            name: "camera_capture".into(),
            description: "Take a photo with the device camera".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "camera": { "type": "string", "enum": ["rear", "front"], "default": "rear" },
                    "flash": { "type": "string", "enum": ["auto", "on", "off"], "default": "auto" }
                }
            }),
        },
        Tool {
            name: "app_launch".into(),
            description: "Launch an application by package name or common name".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "app": { "type": "string", "description": "Package name or app name (e.g. 'chrome', 'com.google.android.gm')" }
                },
                "required": ["app"]
            }),
        },
        Tool {
            name: "alarm_set".into(),
            description: "Set an alarm or timer".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "type": { "type": "string", "enum": ["alarm", "timer"], "description": "Alarm or countdown timer" },
                    "hour": { "type": "integer", "description": "Hour (0-23) for alarm" },
                    "minute": { "type": "integer", "description": "Minute (0-59) for alarm" },
                    "duration_seconds": { "type": "integer", "description": "Duration in seconds for timer" },
                    "label": { "type": "string", "description": "Label for the alarm/timer" }
                },
                "required": ["type"]
            }),
        },
        Tool {
            name: "web_search".into(),
            description: "Perform a web search and return results".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "query": { "type": "string", "description": "Search query" },
                    "num_results": { "type": "integer", "description": "Number of results", "default": 5 }
                },
                "required": ["query"]
            }),
        },
        Tool {
            name: "notification_send".into(),
            description: "Show a notification on the device".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "title": { "type": "string", "description": "Notification title" },
                    "body": { "type": "string", "description": "Notification body text" },
                    "priority": { "type": "string", "enum": ["low", "default", "high"], "default": "default" }
                },
                "required": ["title", "body"]
            }),
        },
        Tool {
            name: "clipboard_get".into(),
            description: "Get current clipboard contents".into(),
            input_schema: json!({ "type": "object", "properties": {} }),
        },
        Tool {
            name: "clipboard_set".into(),
            description: "Set clipboard contents".into(),
            input_schema: json!({
                "type": "object",
                "properties": {
                    "text": { "type": "string", "description": "Text to copy to clipboard" }
                },
                "required": ["text"]
            }),
        },
    ]
}

/// Execute a tool call and return the result.
///
/// In production, each tool dispatches to the appropriate MSI event
/// or system service. For the skeleton, we return structured responses
/// indicating what would happen.
pub fn execute_tool(name: &str, args: &Value) -> ToolResult {
    match name {
        "phone_dial" => {
            let number = args["number"].as_str().unwrap_or("");
            let slot = args["sim_slot"].as_i64().unwrap_or(0);
            if number.is_empty() {
                return ToolResult::error("Phone number is required");
            }
            // TODO: publish MSI event phone/cmd/dial
            ToolResult::json(&json!({
                "status": "dialing",
                "number": number,
                "sim_slot": slot,
                "call_id": 1
            }))
        }

        "phone_answer" => {
            let call_id = args["call_id"].as_i64().unwrap_or(0);
            // TODO: publish MSI event phone/cmd/answer
            ToolResult::json(&json!({
                "status": "answered",
                "call_id": call_id
            }))
        }

        "phone_hangup" => {
            let call_id = args["call_id"].as_i64().unwrap_or(0);
            // TODO: publish MSI event phone/cmd/hangup
            ToolResult::json(&json!({
                "status": "hung_up",
                "call_id": call_id
            }))
        }

        "sms_send" => {
            let to = args["to"].as_str().unwrap_or("");
            let body = args["body"].as_str().unwrap_or("");
            if to.is_empty() || body.is_empty() {
                return ToolResult::error("'to' and 'body' are required");
            }
            // TODO: publish MSI event phone/cmd/sms_send
            ToolResult::json(&json!({
                "status": "sent",
                "to": to,
                "body_length": body.len()
            }))
        }

        "sms_read" => {
            let contact = args["contact"].as_str();
            let unread_only = args["unread_only"].as_bool().unwrap_or(false);
            let limit = args["limit"].as_i64().unwrap_or(10);
            // TODO: query SmsManager via MSI state
            ToolResult::json(&json!({
                "messages": [],
                "contact": contact,
                "unread_only": unread_only,
                "limit": limit,
                "total_unread": 0
            }))
        }

        "sms_search" => {
            let query = args["query"].as_str().unwrap_or("");
            let limit = args["limit"].as_i64().unwrap_or(20);
            // TODO: query SmsManager.search()
            ToolResult::json(&json!({
                "query": query,
                "results": [],
                "limit": limit
            }))
        }

        "settings_wifi" => {
            let enabled = args["enabled"].as_bool().unwrap_or(true);
            // TODO: publish MSI event system/cmd/wifi
            ToolResult::json(&json!({
                "wifi": if enabled { "enabled" } else { "disabled" }
            }))
        }

        "settings_bluetooth" => {
            let enabled = args["enabled"].as_bool().unwrap_or(true);
            ToolResult::json(&json!({
                "bluetooth": if enabled { "enabled" } else { "disabled" }
            }))
        }

        "settings_brightness" => {
            let level = args["level"].as_i64().unwrap_or(128);
            let auto = args["auto"].as_bool().unwrap_or(false);
            ToolResult::json(&json!({
                "brightness": level,
                "auto_brightness": auto
            }))
        }

        "settings_volume" => {
            let stream = args["stream"].as_str().unwrap_or("media");
            let level = args["level"].as_i64().unwrap_or(7);
            ToolResult::json(&json!({
                "stream": stream,
                "volume": level
            }))
        }

        "camera_capture" => {
            let camera = args["camera"].as_str().unwrap_or("rear");
            let flash = args["flash"].as_str().unwrap_or("auto");
            // TODO: publish MSI event camera/cmd/capture
            ToolResult::json(&json!({
                "status": "captured",
                "camera": camera,
                "flash": flash,
                "path": "/sdcard/DCIM/ninjamagic/capture_001.jpg"
            }))
        }

        "app_launch" => {
            let app = args["app"].as_str().unwrap_or("");
            if app.is_empty() {
                return ToolResult::error("App name or package is required");
            }
            ToolResult::json(&json!({
                "status": "launched",
                "app": app
            }))
        }

        "alarm_set" => {
            let alarm_type = args["type"].as_str().unwrap_or("alarm");
            let label = args["label"].as_str().unwrap_or("");
            match alarm_type {
                "alarm" => {
                    let hour = args["hour"].as_i64().unwrap_or(8);
                    let minute = args["minute"].as_i64().unwrap_or(0);
                    ToolResult::json(&json!({
                        "status": "set",
                        "type": "alarm",
                        "time": format!("{:02}:{:02}", hour, minute),
                        "label": label
                    }))
                }
                "timer" => {
                    let duration = args["duration_seconds"].as_i64().unwrap_or(60);
                    ToolResult::json(&json!({
                        "status": "set",
                        "type": "timer",
                        "duration_seconds": duration,
                        "label": label
                    }))
                }
                _ => ToolResult::error("type must be 'alarm' or 'timer'"),
            }
        }

        "web_search" => {
            let query = args["query"].as_str().unwrap_or("");
            let num = args["num_results"].as_i64().unwrap_or(5);
            // TODO: actual web search via system browser or API
            ToolResult::json(&json!({
                "query": query,
                "results": [],
                "num_results": num
            }))
        }

        "notification_send" => {
            let title = args["title"].as_str().unwrap_or("");
            let body = args["body"].as_str().unwrap_or("");
            let priority = args["priority"].as_str().unwrap_or("default");
            ToolResult::json(&json!({
                "status": "posted",
                "title": title,
                "body": body,
                "priority": priority
            }))
        }

        "clipboard_get" => {
            // TODO: read system clipboard via MSI state
            ToolResult::json(&json!({
                "text": "",
                "has_content": false
            }))
        }

        "clipboard_set" => {
            let text = args["text"].as_str().unwrap_or("");
            ToolResult::json(&json!({
                "status": "copied",
                "length": text.len()
            }))
        }

        _ => ToolResult::error(&format!("Unknown tool: {}", name)),
    }
}
