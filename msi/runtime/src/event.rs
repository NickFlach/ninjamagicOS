//! MSI Event Bus — Topic-Based Pub/Sub
//!
//! The event bus is the nervous system of ninjamagicOS. Every phone
//! subsystem (telephony, sensors, camera, power) publishes events
//! on named topics, and cognitive programs subscribe via prefix matching.

use std::os::unix::io::RawFd;
use std::sync::Arc;
use std::time::Duration;
use parking_lot::Mutex;

use crate::ffi::{self, str_to_buf, buf_to_str};
use crate::error::{MsiError, Result};

/// Event delivery quality of service.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum QoS {
    /// Fire and forget — no delivery guarantee.
    BestEffort = 0,
    /// Retry until acknowledged by subscriber.
    AtLeastOnce = 1,
    /// Transactional delivery (future).
    ExactlyOnce = 2,
}

/// A received event from the MSI event bus.
#[derive(Debug, Clone)]
pub struct Event {
    pub id: u64,
    pub topic: String,
    pub ts_nanos: u64,
    pub payload: Vec<u8>,
}

impl Event {
    /// Parse the payload as a UTF-8 string.
    pub fn payload_str(&self) -> Option<&str> {
        std::str::from_utf8(&self.payload).ok()
    }

    /// Parse the payload as JSON.
    pub fn payload_json<T: serde::de::DeserializeOwned>(&self) -> std::result::Result<T, serde_json::Error> {
        serde_json::from_slice(&self.payload)
    }

    /// Get event age relative to current monotonic time.
    pub fn age(&self) -> Duration {
        let now = nix::time::clock_gettime(nix::time::ClockId::CLOCK_MONOTONIC)
            .map(|ts| ts.tv_sec() as u64 * 1_000_000_000 + ts.tv_nsec() as u64)
            .unwrap_or(0);
        if now > self.ts_nanos {
            Duration::from_nanos(now - self.ts_nanos)
        } else {
            Duration::ZERO
        }
    }
}

/// Handle to the MSI event bus for publishing events.
#[derive(Clone)]
pub struct EventBus {
    fd: Arc<Mutex<RawFd>>,
}

impl EventBus {
    pub(crate) fn new(fd: Arc<Mutex<RawFd>>) -> Self {
        EventBus { fd }
    }

    /// Publish an event with a byte payload.
    pub fn publish(
        &self,
        domain_id: u32,
        topic: &str,
        payload: &[u8],
        qos: QoS,
    ) -> Result<u64> {
        if payload.len() > 1024 * 1024 {
            return Err(MsiError::PayloadTooLarge(payload.len()));
        }

        let fd = *self.fd.lock();

        let mut args = ffi::MsiEventPublishRaw {
            domain_id,
            topic: str_to_buf(topic),
            payload_ptr: payload.as_ptr() as u64,
            payload_len: payload.len() as u32,
            qos: qos as u32,
            event_id: 0,
        };

        unsafe {
            ffi::msi_ioc_event_publish(fd, &args)
                .map_err(|e| MsiError::Ioctl(format!("event_publish: {}", e)))?;
        }

        Ok(args.event_id)
    }

    /// Publish an event with a string payload.
    pub fn emit(&self, domain_id: u32, topic: &str, payload: &str) -> Result<u64> {
        self.publish(domain_id, topic, payload.as_bytes(), QoS::BestEffort)
    }

    /// Publish an event with a JSON-serializable payload.
    pub fn emit_json<T: serde::Serialize>(
        &self,
        domain_id: u32,
        topic: &str,
        value: &T,
    ) -> Result<u64> {
        let bytes = serde_json::to_vec(value)
            .map_err(|e| MsiError::Ioctl(format!("json serialize: {}", e)))?;
        self.publish(domain_id, topic, &bytes, QoS::BestEffort)
    }

    /// Create a subscription to events matching a topic prefix.
    pub fn subscribe(
        &self,
        domain_id: u32,
        prefix: &str,
    ) -> Result<Subscription> {
        self.subscribe_filtered(domain_id, prefix, None)
    }

    /// Create a filtered subscription.
    pub fn subscribe_filtered(
        &self,
        domain_id: u32,
        prefix: &str,
        filter: Option<&str>,
    ) -> Result<Subscription> {
        let fd_val = *self.fd.lock();

        let mut args = ffi::MsiEventSubscribeRaw {
            domain_id,
            prefix: str_to_buf(prefix),
            filter: if let Some(f) = filter {
                str_to_buf(f)
            } else {
                [0u8; 256]
            },
            sub_id: 0,
        };

        unsafe {
            ffi::msi_ioc_event_subscribe(fd_val, &mut args)
                .map_err(|e| MsiError::Ioctl(format!("event_subscribe: {}", e)))?;
        }

        Ok(Subscription {
            fd: self.fd.clone(),
            id: args.sub_id,
            prefix: prefix.to_string(),
        })
    }
}

/// An active event subscription. Receives events matching the prefix.
pub struct Subscription {
    fd: Arc<Mutex<RawFd>>,
    id: u32,
    prefix: String,
}

impl Subscription {
    /// Get the subscription ID.
    pub fn id(&self) -> u32 {
        self.id
    }

    /// Get the topic prefix this subscription matches.
    pub fn prefix(&self) -> &str {
        &self.prefix
    }

    /// Wait for the next event, blocking until one arrives or timeout.
    ///
    /// Returns `None` on timeout.
    pub fn wait(&self, timeout: Duration) -> Result<Option<Event>> {
        self.wait_nanos(timeout.as_nanos() as u64)
    }

    /// Wait for the next event with a nanosecond timeout.
    /// Pass 0 for infinite wait.
    pub fn wait_nanos(&self, timeout_nanos: u64) -> Result<Option<Event>> {
        let fd = *self.fd.lock();

        // Allocate a payload buffer (64KB should cover most events)
        let mut payload_buf = vec![0u8; 65536];

        let mut args = ffi::MsiEventWaitRaw {
            sub_id: self.id,
            timeout_nanos,
            event_id: 0,
            topic: [0u8; 256],
            ts_nanos: 0,
            payload_len: payload_buf.len() as u32,
            payload_ptr: payload_buf.as_mut_ptr() as u64,
        };

        let result = unsafe {
            ffi::msi_ioc_event_wait(fd, &mut args)
        };

        match result {
            Ok(_) => {
                payload_buf.truncate(args.payload_len as usize);
                Ok(Some(Event {
                    id: args.event_id,
                    topic: buf_to_str(&args.topic),
                    ts_nanos: args.ts_nanos,
                    payload: payload_buf,
                }))
            }
            Err(nix::errno::Errno::ETIMEDOUT) => Ok(None),
            Err(nix::errno::Errno::EINTR) => Ok(None),
            Err(nix::errno::Errno::EAGAIN) => Ok(None),
            Err(e) => Err(MsiError::Ioctl(format!("event_wait: {}", e))),
        }
    }

    /// Non-blocking poll — returns immediately with any pending event.
    pub fn poll(&self) -> Result<Option<Event>> {
        self.wait_nanos(1) // 1 nanosecond timeout = effectively non-blocking
    }

    /// Acknowledge an event (for at_least_once QoS).
    pub fn ack(&self, event_id: u64) -> Result<()> {
        let fd = *self.fd.lock();
        unsafe {
            ffi::msi_ioc_event_ack(fd, &event_id)
                .map_err(|e| MsiError::Ioctl(format!("event_ack: {}", e)))?;
        }
        Ok(())
    }

    /// Blocking iterator over events. Yields events until the subscription
    /// is dropped or the thread is interrupted.
    pub fn iter(&self) -> SubscriptionIter<'_> {
        SubscriptionIter { sub: self }
    }
}

/// Blocking iterator over subscription events.
pub struct SubscriptionIter<'a> {
    sub: &'a Subscription,
}

impl<'a> Iterator for SubscriptionIter<'a> {
    type Item = Event;

    fn next(&mut self) -> Option<Self::Item> {
        // Block for up to 1 second, then retry
        loop {
            match self.sub.wait(Duration::from_secs(1)) {
                Ok(Some(event)) => return Some(event),
                Ok(None) => continue, // timeout, retry
                Err(_) => return None, // error, stop
            }
        }
    }
}
