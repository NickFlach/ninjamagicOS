//! Intent Classification — routes user input to skills
//!
//! Lightweight intent classifier that runs on-device. In production,
//! this uses a tiny DistilBERT model on the NPU/DSP for fast
//! classification (<100ms). For now, uses keyword matching as a
//! placeholder until the inference engine is integrated.

use serde::{Serialize, Deserialize};

/// Classified user intent.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Intent {
    pub category: IntentCategory,
    pub action: String,
    pub confidence: f32,
    pub entities: Vec<Entity>,
}

/// High-level intent categories mapping to skill groups.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum IntentCategory {
    PhoneCall,
    Sms,
    Contacts,
    Settings,
    Apps,
    Media,
    Alarm,
    Camera,
    Web,
    Files,
    Navigation,
    General,
    Unknown,
}

/// Extracted entity from user input.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Entity {
    pub kind: EntityKind,
    pub value: String,
    pub span: (usize, usize),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum EntityKind {
    PhoneNumber,
    ContactName,
    AppName,
    Url,
    Time,
    Duration,
    Location,
    Text,
}

/// Classify user input into an intent.
///
/// Current implementation: keyword matching.
/// Target implementation: DistilBERT-tiny on NPU (<100ms).
pub fn classify(input: &str) -> Intent {
    let lower = input.to_lowercase();

    // Phone call intents
    if lower.contains("call") || lower.contains("dial") || lower.contains("ring") {
        let number = extract_phone_number(input);
        let name = extract_after_keyword(input, &["call", "dial"]);
        let mut entities = Vec::new();

        if let Some(num) = number {
            entities.push(Entity {
                kind: EntityKind::PhoneNumber,
                value: num,
                span: (0, 0),
            });
        } else if let Some(n) = name {
            entities.push(Entity {
                kind: EntityKind::ContactName,
                value: n,
                span: (0, 0),
            });
        }

        return Intent {
            category: IntentCategory::PhoneCall,
            action: "dial".into(),
            confidence: 0.85,
            entities,
        };
    }

    // SMS intents
    if lower.contains("text") || lower.contains("message") || lower.contains("sms") {
        return Intent {
            category: IntentCategory::Sms,
            action: if lower.contains("read") || lower.contains("show") {
                "read".into()
            } else {
                "send".into()
            },
            confidence: 0.80,
            entities: Vec::new(),
        };
    }

    // Settings intents
    if lower.contains("wifi") || lower.contains("bluetooth") ||
       lower.contains("brightness") || lower.contains("volume") ||
       lower.contains("airplane") || lower.contains("setting") {
        let action = if lower.contains("wifi") {
            "wifi"
        } else if lower.contains("bluetooth") {
            "bluetooth"
        } else if lower.contains("brightness") {
            "brightness"
        } else if lower.contains("volume") {
            "volume"
        } else {
            "general"
        };

        return Intent {
            category: IntentCategory::Settings,
            action: action.into(),
            confidence: 0.85,
            entities: Vec::new(),
        };
    }

    // Camera intents
    if lower.contains("photo") || lower.contains("picture") ||
       lower.contains("camera") || lower.contains("selfie") ||
       lower.contains("screenshot") {
        return Intent {
            category: IntentCategory::Camera,
            action: if lower.contains("screenshot") {
                "screenshot".into()
            } else {
                "capture".into()
            },
            confidence: 0.85,
            entities: Vec::new(),
        };
    }

    // Alarm/timer intents
    if lower.contains("alarm") || lower.contains("timer") ||
       lower.contains("remind") || lower.contains("wake") {
        return Intent {
            category: IntentCategory::Alarm,
            action: "set".into(),
            confidence: 0.80,
            entities: Vec::new(),
        };
    }

    // Web intents
    if lower.contains("search") || lower.contains("google") ||
       lower.contains("look up") || lower.contains("find") ||
       lower.starts_with("what") || lower.starts_with("who") ||
       lower.starts_with("how") || lower.starts_with("where") {
        return Intent {
            category: IntentCategory::Web,
            action: "search".into(),
            confidence: 0.70,
            entities: vec![Entity {
                kind: EntityKind::Text,
                value: input.to_string(),
                span: (0, input.len()),
            }],
        };
    }

    // App intents
    if lower.contains("open") || lower.contains("launch") || lower.contains("start") {
        let app_name = extract_after_keyword(input, &["open", "launch", "start"]);
        return Intent {
            category: IntentCategory::Apps,
            action: "open".into(),
            confidence: 0.80,
            entities: app_name.map(|n| vec![Entity {
                kind: EntityKind::AppName,
                value: n,
                span: (0, 0),
            }]).unwrap_or_default(),
        };
    }

    // Media intents
    if lower.contains("play") || lower.contains("pause") || lower.contains("music") ||
       lower.contains("next") || lower.contains("previous") || lower.contains("stop") {
        return Intent {
            category: IntentCategory::Media,
            action: if lower.contains("pause") || lower.contains("stop") {
                "pause".into()
            } else if lower.contains("next") {
                "next".into()
            } else {
                "play".into()
            },
            confidence: 0.80,
            entities: Vec::new(),
        };
    }

    // Default: unknown — will fall through to LLM
    Intent {
        category: IntentCategory::Unknown,
        action: "general".into(),
        confidence: 0.0,
        entities: vec![Entity {
            kind: EntityKind::Text,
            value: input.to_string(),
            span: (0, input.len()),
        }],
    }
}

/// Extract a phone number pattern from text.
fn extract_phone_number(input: &str) -> Option<String> {
    let digits: String = input.chars()
        .filter(|c| c.is_ascii_digit() || *c == '+' || *c == '-' || *c == '(' || *c == ')')
        .collect();

    let pure_digits: String = digits.chars().filter(|c| c.is_ascii_digit()).collect();
    if pure_digits.len() >= 7 {
        Some(digits)
    } else {
        None
    }
}

/// Extract text after a keyword (e.g., "call Mom" → "Mom").
fn extract_after_keyword(input: &str, keywords: &[&str]) -> Option<String> {
    let lower = input.to_lowercase();
    for kw in keywords {
        if let Some(pos) = lower.find(kw) {
            let after = &input[pos + kw.len()..].trim();
            if !after.is_empty() {
                return Some(after.to_string());
            }
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_classify_call() {
        let intent = classify("Call Mom");
        assert_eq!(intent.category, IntentCategory::PhoneCall);
        assert_eq!(intent.action, "dial");
    }

    #[test]
    fn test_classify_sms() {
        let intent = classify("Send a text message");
        assert_eq!(intent.category, IntentCategory::Sms);
        assert_eq!(intent.action, "send");
    }

    #[test]
    fn test_classify_wifi() {
        let intent = classify("Turn on wifi");
        assert_eq!(intent.category, IntentCategory::Settings);
        assert_eq!(intent.action, "wifi");
    }

    #[test]
    fn test_classify_unknown() {
        let intent = classify("Tell me a joke");
        assert_eq!(intent.category, IntentCategory::Unknown);
    }

    #[test]
    fn test_extract_phone_number() {
        assert_eq!(extract_phone_number("Call 555-1234"), Some("555-1234".into()));
        assert_eq!(extract_phone_number("Call +1 (555) 123-4567"),
                   Some("+1(555)123-4567".into()));
        assert_eq!(extract_phone_number("Call Mom"), None);
    }
}
