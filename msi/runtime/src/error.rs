//! MSI error types

use thiserror::Error;

#[derive(Error, Debug)]
pub enum MsiError {
    #[error("MSI device not found at /dev/msi — is the kernel module loaded?")]
    DeviceNotFound,

    #[error("Permission denied: {0}")]
    PermissionDenied(String),

    #[error("Domain not found: id={0}")]
    DomainNotFound(u32),

    #[error("Domain sealed: cannot add grants to '{0}'")]
    DomainSealed(String),

    #[error("Lane not found: id={0}")]
    LaneNotFound(u32),

    #[error("Subscription not found: id={0}")]
    SubscriptionNotFound(u32),

    #[error("State region not found: handle={0}")]
    StateNotFound(u32),

    #[error("Access out of bounds: offset={offset} len={len} size={size}")]
    OutOfBounds {
        offset: usize,
        len: usize,
        size: usize,
    },

    #[error("Event wait timed out after {0}ms")]
    Timeout(u64),

    #[error("Payload too large: {0} bytes (max 1MB)")]
    PayloadTooLarge(usize),

    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("ioctl failed: {0}")]
    Ioctl(String),

    #[error("Kernel module error: {0}")]
    Kernel(i32),
}

pub type Result<T> = std::result::Result<T, MsiError>;
