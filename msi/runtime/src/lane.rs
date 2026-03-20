//! MSI Lane — Execution Context
//!
//! Lanes are cognitive execution contexts backed by kernel threads
//! with scheduling policies (priority, energy budget, CPU affinity).
//! The NinjaMagic Agent core loop, each skill, and background tasks
//! all run as separate MSI lanes.

use std::os::unix::io::RawFd;
use std::sync::Arc;
use parking_lot::Mutex;

use crate::ffi::{self, str_to_buf, MsiLanePolicyRaw, MsiLaneSpawnRaw};
use crate::error::{MsiError, Result};

/// Lane scheduling priority.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum Priority {
    Low = 0,
    Normal = 1,
    High = 2,
    Realtime = 3,
}

/// Lane energy budget hint.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum EnergyBudget {
    /// Minimize power consumption (efficiency cores, throttled).
    Low = 0,
    /// Default trade-off between performance and power.
    Balanced = 1,
    /// Maximum performance, no power restrictions.
    Unbounded = 2,
}

/// Lane CPU/accelerator affinity.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum Affinity {
    /// No preference — scheduler decides.
    Any = 0,
    /// Pin to efficiency (LITTLE) cores.
    Little = 1,
    /// Pin to performance (big) cores.
    Big = 2,
    /// Pin to big cores; hint for NPU dispatch (Tensor TPU / Hexagon).
    Npu = 3,
    /// Pin to big cores; hint for GPU compute dispatch.
    Gpu = 4,
    /// Pin to big cores; hint for DSP dispatch (Hexagon).
    Dsp = 5,
}

/// Lane scheduling policy.
#[derive(Debug, Clone, Copy)]
pub struct LanePolicy {
    pub priority: Priority,
    pub energy: EnergyBudget,
    pub affinity: Affinity,
}

impl Default for LanePolicy {
    fn default() -> Self {
        LanePolicy {
            priority: Priority::Normal,
            energy: EnergyBudget::Balanced,
            affinity: Affinity::Any,
        }
    }
}

impl LanePolicy {
    pub fn new(priority: Priority, energy: EnergyBudget, affinity: Affinity) -> Self {
        LanePolicy { priority, energy, affinity }
    }

    fn to_raw(&self) -> MsiLanePolicyRaw {
        MsiLanePolicyRaw {
            priority: self.priority as u32,
            energy: self.energy as u32,
            affinity: self.affinity as u32,
        }
    }
}

/// A spawned MSI lane handle.
pub struct Lane {
    fd: Arc<Mutex<RawFd>>,
    id: u32,
    domain_id: u32,
    entry: String,
    policy: LanePolicy,
}

impl Lane {
    /// Spawn a new lane in the given domain.
    pub fn spawn(
        fd: Arc<Mutex<RawFd>>,
        domain_id: u32,
        entry: &str,
        policy: LanePolicy,
    ) -> Result<Self> {
        let raw_fd = *fd.lock();

        let mut args = MsiLaneSpawnRaw {
            domain_id,
            entry: str_to_buf(entry),
            policy: policy.to_raw(),
            lane_id: 0,
        };

        unsafe {
            ffi::msi_ioc_lane_spawn(raw_fd, &mut args)
                .map_err(|e| MsiError::Ioctl(format!("lane_spawn: {}", e)))?;
        }

        Ok(Lane {
            fd,
            id: args.lane_id,
            domain_id,
            entry: entry.to_string(),
            policy,
        })
    }

    /// Get the lane ID.
    pub fn id(&self) -> u32 {
        self.id
    }

    /// Get the domain ID this lane belongs to.
    pub fn domain_id(&self) -> u32 {
        self.domain_id
    }

    /// Get the entry point name.
    pub fn entry(&self) -> &str {
        &self.entry
    }

    /// Get the current policy.
    pub fn policy(&self) -> &LanePolicy {
        &self.policy
    }

    /// Cooperative yield — give up the CPU timeslice.
    pub fn yield_now(&self) -> Result<()> {
        let fd = *self.fd.lock();
        unsafe {
            ffi::msi_ioc_lane_yield(fd, &self.id)
                .map_err(|e| MsiError::Ioctl(format!("lane_yield: {}", e)))?;
        }
        Ok(())
    }

    /// Sleep the lane for the specified duration.
    pub fn sleep(&self, duration: std::time::Duration) -> Result<()> {
        let nanos = duration.as_nanos() as u64;
        let fd = *self.fd.lock();
        unsafe {
            ffi::msi_ioc_lane_sleep(fd, &nanos)
                .map_err(|e| MsiError::Ioctl(format!("lane_sleep: {}", e)))?;
        }
        Ok(())
    }

    /// Kill this lane (terminates the kernel thread).
    pub fn kill(self) -> Result<()> {
        let fd = *self.fd.lock();
        unsafe {
            ffi::msi_ioc_lane_kill(fd, &self.id)
                .map_err(|e| MsiError::Ioctl(format!("lane_kill: {}", e)))?;
        }
        Ok(())
    }
}
