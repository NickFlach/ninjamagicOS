//! SMS Manager — tracks SMS conversations via MSI events
//!
//! Subscribes to phone/sms/* events from the RILD bridge and
//! maintains a conversation-threaded message store. Provides
//! search and retrieval for the agent.

use std::collections::HashMap;
use serde::{Serialize, Deserialize};
use parking_lot::RwLock;
use log::info;

/// SMS direction.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum SmsDirection {
    Received,
    Sent,
}

/// SMS delivery status.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum SmsStatus {
    Pending,
    Delivered,
    Failed,
}

/// A single SMS message.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SmsMessage {
    pub id: u64,
    pub contact: String,
    pub body: String,
    pub direction: SmsDirection,
    pub timestamp_ms: u64,
    pub status: SmsStatus,
    pub sim_slot: i32,
}

/// A conversation thread (grouped by contact number).
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Conversation {
    pub contact: String,
    pub messages: Vec<SmsMessage>,
    pub unread_count: u32,
    pub last_activity_ms: u64,
}

impl Conversation {
    fn new(contact: &str) -> Self {
        Conversation {
            contact: contact.to_string(),
            messages: Vec::new(),
            unread_count: 0,
            last_activity_ms: 0,
        }
    }

    fn add_message(&mut self, msg: SmsMessage) {
        if msg.direction == SmsDirection::Received {
            self.unread_count += 1;
        }
        self.last_activity_ms = msg.timestamp_ms;
        self.messages.push(msg);
    }

    /// Get the last N messages in this conversation.
    pub fn recent(&self, n: usize) -> &[SmsMessage] {
        let start = self.messages.len().saturating_sub(n);
        &self.messages[start..]
    }

    /// Mark all messages as read.
    pub fn mark_read(&mut self) {
        self.unread_count = 0;
    }
}

/// SMS manager — maintains conversation threads.
pub struct SmsManager {
    conversations: RwLock<HashMap<String, Conversation>>,
    next_id: RwLock<u64>,
}

impl SmsManager {
    pub fn new() -> Self {
        SmsManager {
            conversations: RwLock::new(HashMap::new()),
            next_id: RwLock::new(1),
        }
    }

    fn alloc_id(&self) -> u64 {
        let mut id = self.next_id.write();
        let current = *id;
        *id += 1;
        current
    }

    fn now_ms() -> u64 {
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map(|d| d.as_millis() as u64)
            .unwrap_or(0)
    }

    /// Normalize a phone number for conversation grouping.
    fn normalize_number(number: &str) -> String {
        // Strip non-digit characters except leading +
        let mut normalized = String::new();
        for (i, c) in number.chars().enumerate() {
            if c.is_ascii_digit() || (i == 0 && c == '+') {
                normalized.push(c);
            }
        }
        // If number has 10 digits and no country code, prepend +1
        let digits: String = normalized.chars().filter(|c| c.is_ascii_digit()).collect();
        if digits.len() == 10 && !normalized.starts_with('+') {
            format!("+1{}", digits)
        } else {
            normalized
        }
    }

    /// Process a received SMS from the RILD bridge.
    pub fn on_received(&self, from: &str, body: &str, timestamp_ms: u64) {
        let contact = Self::normalize_number(from);
        let ts = if timestamp_ms > 0 { timestamp_ms } else { Self::now_ms() };

        let msg = SmsMessage {
            id: self.alloc_id(),
            contact: contact.clone(),
            body: body.to_string(),
            direction: SmsDirection::Received,
            timestamp_ms: ts,
            status: SmsStatus::Delivered,
            sim_slot: 0,
        };

        info!("SMS received: from={} body_len={}", contact, body.len());

        let mut convos = self.conversations.write();
        convos.entry(contact.clone())
            .or_insert_with(|| Conversation::new(&contact))
            .add_message(msg);
    }

    /// Process a sent SMS confirmation.
    pub fn on_sent(&self, to: &str, body: &str, status: SmsStatus) {
        let contact = Self::normalize_number(to);

        let msg = SmsMessage {
            id: self.alloc_id(),
            contact: contact.clone(),
            body: body.to_string(),
            direction: SmsDirection::Sent,
            timestamp_ms: Self::now_ms(),
            status,
            sim_slot: 0,
        };

        info!("SMS sent: to={} status={:?}", contact, status);

        let mut convos = self.conversations.write();
        convos.entry(contact.clone())
            .or_insert_with(|| Conversation::new(&contact))
            .add_message(msg);
    }

    /// Get a conversation by contact number.
    pub fn get_conversation(&self, number: &str) -> Option<Conversation> {
        let contact = Self::normalize_number(number);
        self.conversations.read().get(&contact).cloned()
    }

    /// Get all conversations sorted by last activity (most recent first).
    pub fn all_conversations(&self) -> Vec<Conversation> {
        let convos = self.conversations.read();
        let mut list: Vec<Conversation> = convos.values().cloned().collect();
        list.sort_by(|a, b| b.last_activity_ms.cmp(&a.last_activity_ms));
        list
    }

    /// Get conversations with unread messages.
    pub fn unread_conversations(&self) -> Vec<Conversation> {
        self.conversations.read().values()
            .filter(|c| c.unread_count > 0)
            .cloned()
            .collect()
    }

    /// Get total unread message count.
    pub fn total_unread(&self) -> u32 {
        self.conversations.read().values()
            .map(|c| c.unread_count)
            .sum()
    }

    /// Mark a conversation as read.
    pub fn mark_read(&self, number: &str) {
        let contact = Self::normalize_number(number);
        if let Some(convo) = self.conversations.write().get_mut(&contact) {
            convo.mark_read();
        }
    }

    /// Search messages by body text (case-insensitive).
    pub fn search(&self, query: &str) -> Vec<SmsMessage> {
        let lower_query = query.to_lowercase();
        let convos = self.conversations.read();
        convos.values()
            .flat_map(|c| c.messages.iter())
            .filter(|m| m.body.to_lowercase().contains(&lower_query))
            .cloned()
            .collect()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_normalize_number() {
        assert_eq!(SmsManager::normalize_number("555-123-4567"), "+15551234567");
        assert_eq!(SmsManager::normalize_number("+1 (555) 123-4567"), "+15551234567");
        assert_eq!(SmsManager::normalize_number("+442071234567"), "+442071234567");
    }

    #[test]
    fn test_sms_conversation() {
        let mgr = SmsManager::new();
        mgr.on_received("+15551234567", "Hello!", 1000);
        mgr.on_received("+15551234567", "How are you?", 2000);
        mgr.on_sent("+15551234567", "Good thanks!", SmsStatus::Delivered);

        let convo = mgr.get_conversation("+15551234567").unwrap();
        assert_eq!(convo.messages.len(), 3);
        assert_eq!(convo.unread_count, 2); // 2 received, not marked read
    }

    #[test]
    fn test_unread_count() {
        let mgr = SmsManager::new();
        mgr.on_received("555-111-2222", "Hey", 1000);
        mgr.on_received("555-333-4444", "What's up", 2000);
        assert_eq!(mgr.total_unread(), 2);

        mgr.mark_read("555-111-2222");
        assert_eq!(mgr.total_unread(), 1);
    }
}
