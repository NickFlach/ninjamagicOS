//! Substrate — top-level MSI runtime handle
//!
//! The Substrate is the entry point for all MSI operations.
//! It opens /dev/msi, probes hardware capabilities, and provides
//! factory methods for creating domains, lanes, subscriptions, etc.

use std::os::unix::io::RawFd;
use std::sync::Arc;
use parking_lot::Mutex;
use log::info;

use crate::ffi;
use crate::error::{MsiError, Result};
use crate::domain::DomainBuilder;
use crate::event::EventBus;

/// Hardware capabilities reported by the MSI kernel module.
#[derive(Debug, Clone)]
pub struct Capabilities {
    pub lanes_min: u32,
    pub lanes_max: u32,
    pub lanes_realtime: bool,
    pub events_max_topics: u32,
    pub state_max_bytes: u64,
    pub security_attest: bool,
    pub security_model: SecurityModel,
    pub accel_cpu: bool,
    pub accel_gpu: bool,
    pub accel_npu: bool,
    pub accel_dsp: bool,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SecurityModel {
    None,
    AppSandbox,
    Tee,
    SecureEnclave,
}

impl From<u32> for SecurityModel {
    fn from(v: u32) -> Self {
        match v {
            1 => SecurityModel::AppSandbox,
            2 => SecurityModel::Tee,
            3 => SecurityModel::SecureEnclave,
            _ => SecurityModel::None,
        }
    }
}

/// MSI version (major.minor.patch packed as u32).
#[derive(Debug, Clone, Copy)]
pub struct Version {
    pub major: u8,
    pub minor: u8,
    pub patch: u8,
}

impl From<u32> for Version {
    fn from(v: u32) -> Self {
        Version {
            major: ((v >> 16) & 0xFF) as u8,
            minor: ((v >> 8) & 0xFF) as u8,
            patch: (v & 0xFF) as u8,
        }
    }
}

impl std::fmt::Display for Version {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}.{}.{}", self.major, self.minor, self.patch)
    }
}

/// The MSI Substrate — main runtime handle.
///
/// Owns the file descriptor to /dev/msi and provides all MSI operations.
/// Thread-safe via internal locking.
pub struct Substrate {
    fd: Arc<Mutex<RawFd>>,
    version: Version,
    capabilities: Capabilities,
}

impl Substrate {
    /// Connect to the MSI kernel module.
    ///
    /// This is the MSI boot sequence Phase 0 (Substrate Probe):
    /// 1. Open /dev/msi
    /// 2. Query version
    /// 3. Query capabilities
    pub fn connect() -> Result<Self> {
        let fd = ffi::open_msi_device().map_err(|e| {
            if e.kind() == std::io::ErrorKind::NotFound {
                MsiError::DeviceNotFound
            } else {
                MsiError::Io(e)
            }
        })?;

        // Query version
        let mut version_raw: u32 = 0;
        unsafe {
            ffi::msi_ioc_version(fd, &mut version_raw)
                .map_err(|e| MsiError::Ioctl(format!("version: {}", e)))?;
        }
        let version = Version::from(version_raw);

        // Query capabilities
        let mut caps_raw = ffi::MsiCapabilitiesRaw::default();
        unsafe {
            ffi::msi_ioc_capabilities(fd, &mut caps_raw)
                .map_err(|e| MsiError::Ioctl(format!("capabilities: {}", e)))?;
        }

        let capabilities = Capabilities {
            lanes_min: caps_raw.lanes_min,
            lanes_max: caps_raw.lanes_max,
            lanes_realtime: caps_raw.lanes_realtime != 0,
            events_max_topics: caps_raw.events_max_topics,
            state_max_bytes: caps_raw.state_max_bytes,
            security_attest: caps_raw.security_attest != 0,
            security_model: SecurityModel::from(caps_raw.security_model),
            accel_cpu: caps_raw.accel_cpu != 0,
            accel_gpu: caps_raw.accel_gpu != 0,
            accel_npu: caps_raw.accel_npu != 0,
            accel_dsp: caps_raw.accel_dsp != 0,
        };

        info!(
            "MSI substrate connected — v{} lanes={}-{} npu={} gpu={} dsp={} security={:?}",
            version, capabilities.lanes_min, capabilities.lanes_max,
            capabilities.accel_npu, capabilities.accel_gpu,
            capabilities.accel_dsp, capabilities.security_model
        );

        Ok(Substrate {
            fd: Arc::new(Mutex::new(fd)),
            version,
            capabilities,
        })
    }

    /// Get the MSI version.
    pub fn version(&self) -> Version {
        self.version
    }

    /// Get hardware capabilities.
    pub fn capabilities(&self) -> &Capabilities {
        &self.capabilities
    }

    /// Get the raw file descriptor (for FFI calls in other modules).
    pub(crate) fn fd(&self) -> Arc<Mutex<RawFd>> {
        self.fd.clone()
    }

    /// Create a new domain builder.
    pub fn domain(&self, name: &str) -> DomainBuilder {
        DomainBuilder::new(self.fd.clone(), name)
    }

    /// Get the event bus handle.
    pub fn event_bus(&self) -> EventBus {
        EventBus::new(self.fd.clone())
    }

    /// Check if NPU acceleration is available.
    pub fn has_npu(&self) -> bool {
        self.capabilities.accel_npu
    }

    /// Check if GPU compute is available.
    pub fn has_gpu(&self) -> bool {
        self.capabilities.accel_gpu
    }

    /// Check if DSP is available.
    pub fn has_dsp(&self) -> bool {
        self.capabilities.accel_dsp
    }

    /// Check if hardware attestation is supported.
    pub fn has_attestation(&self) -> bool {
        self.capabilities.security_attest
    }
}

impl Drop for Substrate {
    fn drop(&mut self) {
        let fd = self.fd.lock();
        unsafe {
            libc::close(*fd);
        }
    }
}
