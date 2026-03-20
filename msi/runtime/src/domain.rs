//! MSI Domain — Capability Container
//!
//! Domains enforce least-privilege security for cognitive programs.
//! Each domain holds grants that control access to events, state,
//! associative memory, clock, and accelerators.

use std::os::unix::io::RawFd;
use std::sync::Arc;
use parking_lot::Mutex;

use crate::ffi::{self, str_to_buf};
use crate::error::{MsiError, Result};
use crate::state::Perms;

/// A grant (capability) that can be added to a domain.
#[derive(Debug, Clone)]
pub enum Grant {
    /// Access to event topics matching a prefix.
    Events(String),
    /// Access to a named addressable state region.
    State(String, Perms),
    /// Access to a named associative memory space.
    Assoc(String, Perms),
    /// Access to clock/time operations.
    Clock,
    /// Access to a hardware accelerator ("cpu", "gpu", "npu", "dsp").
    Accel(String),
}

impl Grant {
    fn to_raw(&self) -> ffi::MsiGrantRaw {
        let mut raw = ffi::MsiGrantRaw {
            kind: 0,
            data: [0u8; 256],
        };
        match self {
            Grant::Events(prefix) => {
                raw.kind = 0; // MSI_GRANT_EVENTS
                let bytes = prefix.as_bytes();
                let len = bytes.len().min(255);
                raw.data[..len].copy_from_slice(&bytes[..len]);
            }
            Grant::State(name, perms) => {
                raw.kind = 1; // MSI_GRANT_STATE
                let bytes = name.as_bytes();
                let len = bytes.len().min(127);
                raw.data[..len].copy_from_slice(&bytes[..len]);
                raw.data[128] = *perms as u8;
            }
            Grant::Assoc(space, perms) => {
                raw.kind = 2; // MSI_GRANT_ASSOC
                let bytes = space.as_bytes();
                let len = bytes.len().min(127);
                raw.data[..len].copy_from_slice(&bytes[..len]);
                raw.data[128] = *perms as u8;
            }
            Grant::Clock => {
                raw.kind = 3; // MSI_GRANT_CLOCK
            }
            Grant::Accel(which) => {
                raw.kind = 4; // MSI_GRANT_ACCEL
                let bytes = which.as_bytes();
                let len = bytes.len().min(31);
                raw.data[..len].copy_from_slice(&bytes[..len]);
            }
        }
        raw
    }
}

/// Builder for creating MSI domains with a fluent API.
///
/// ```rust,no_run
/// let domain = substrate.domain("NinjaMagicAgent")
///     .grant(Grant::Events("phone/"))
///     .grant(Grant::Events("sensor/"))
///     .grant(Grant::Assoc("working".into(), Perms::ReadWrite))
///     .grant(Grant::Accel("npu".into()))
///     .grant(Grant::Clock)
///     .seal()
///     .build()?;
/// ```
pub struct DomainBuilder {
    fd: Arc<Mutex<RawFd>>,
    name: String,
    grants: Vec<Grant>,
    should_seal: bool,
}

impl DomainBuilder {
    pub(crate) fn new(fd: Arc<Mutex<RawFd>>, name: &str) -> Self {
        DomainBuilder {
            fd,
            name: name.to_string(),
            grants: Vec::new(),
            should_seal: false,
        }
    }

    /// Add a capability grant.
    pub fn grant(mut self, grant: Grant) -> Self {
        self.grants.push(grant);
        self
    }

    /// Mark the domain to be sealed after creation (immutable grants).
    pub fn seal(mut self) -> Self {
        self.should_seal = true;
        self
    }

    /// Create the domain in the kernel.
    pub fn build(self) -> Result<Domain> {
        let fd = *self.fd.lock();

        // Create domain (without grants — added separately)
        let mut args = ffi::MsiDomainCreateRaw {
            name: str_to_buf(&self.name),
            num_grants: 0,
            seal: 0, // Don't seal yet — add grants first
            domain_id: 0,
        };

        unsafe {
            ffi::msi_ioc_domain_create(fd, &mut args)
                .map_err(|e| MsiError::Ioctl(format!("domain_create: {}", e)))?;
        }

        let domain_id = args.domain_id;

        // Add grants one by one
        for grant in &self.grants {
            let grant_args = ffi::MsiDomainGrantRaw {
                domain_id,
                grant: grant.to_raw(),
            };
            unsafe {
                ffi::msi_ioc_domain_grant(fd, &grant_args)
                    .map_err(|e| MsiError::Ioctl(format!("domain_grant: {}", e)))?;
            }
        }

        // Seal if requested
        if self.should_seal {
            unsafe {
                ffi::msi_ioc_domain_seal(fd, &domain_id)
                    .map_err(|e| MsiError::Ioctl(format!("domain_seal: {}", e)))?;
            }
        }

        Ok(Domain {
            fd: self.fd,
            id: domain_id,
            name: self.name,
            sealed: self.should_seal,
        })
    }
}

/// A created MSI domain with an assigned ID.
#[derive(Debug)]
pub struct Domain {
    fd: Arc<Mutex<RawFd>>,
    id: u32,
    name: String,
    sealed: bool,
}

impl Domain {
    /// Get the domain ID.
    pub fn id(&self) -> u32 {
        self.id
    }

    /// Get the domain name.
    pub fn name(&self) -> &str {
        &self.name
    }

    /// Check if the domain is sealed.
    pub fn is_sealed(&self) -> bool {
        self.sealed
    }

    /// Add a grant to an unsealed domain.
    pub fn add_grant(&mut self, grant: Grant) -> Result<()> {
        if self.sealed {
            return Err(MsiError::DomainSealed(self.name.clone()));
        }
        let fd = *self.fd.lock();
        let args = ffi::MsiDomainGrantRaw {
            domain_id: self.id,
            grant: grant.to_raw(),
        };
        unsafe {
            ffi::msi_ioc_domain_grant(fd, &args)
                .map_err(|e| MsiError::Ioctl(format!("domain_grant: {}", e)))?;
        }
        Ok(())
    }

    /// Seal the domain permanently.
    pub fn seal(&mut self) -> Result<()> {
        let fd = *self.fd.lock();
        unsafe {
            ffi::msi_ioc_domain_seal(fd, &self.id)
                .map_err(|e| MsiError::Ioctl(format!("domain_seal: {}", e)))?;
        }
        self.sealed = true;
        Ok(())
    }
}
