//! MSI Addressable State — Named Memory Regions
//!
//! State regions are named, sized byte buffers that cognitive programs
//! use for working memory. Access is controlled via domain grants.
//! commit() flushes to persistent storage when available.

use std::os::unix::io::RawFd;
use std::sync::Arc;
use parking_lot::Mutex;

use crate::ffi::{self, str_to_buf};
use crate::error::{MsiError, Result};

/// Access permissions for state regions.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum Perms {
    Read = 0,
    ReadWrite = 1,
}

/// A mapped MSI state region.
pub struct StateRegion {
    fd: Arc<Mutex<RawFd>>,
    handle_id: u32,
    name: String,
    size: usize,
    perms: Perms,
}

impl StateRegion {
    /// Map a new state region in the kernel.
    pub fn map(
        fd: Arc<Mutex<RawFd>>,
        domain_id: u32,
        name: &str,
        size: usize,
        perms: Perms,
    ) -> Result<Self> {
        let raw_fd = *fd.lock();

        let mut args = ffi::MsiStateMapRaw {
            domain_id,
            name: str_to_buf(name),
            bytes: size as u64,
            perms: perms as u32,
            handle_id: 0,
        };

        unsafe {
            ffi::msi_ioc_state_map(raw_fd, &mut args)
                .map_err(|e| MsiError::Ioctl(format!("state_map: {}", e)))?;
        }

        Ok(StateRegion {
            fd,
            handle_id: args.handle_id,
            name: name.to_string(),
            size,
            perms,
        })
    }

    /// Get the kernel handle ID.
    pub fn handle(&self) -> u32 {
        self.handle_id
    }

    /// Get the region name.
    pub fn name(&self) -> &str {
        &self.name
    }

    /// Get the region size in bytes.
    pub fn size(&self) -> usize {
        self.size
    }

    /// Read bytes from the region.
    pub fn read(&self, offset: usize, len: usize) -> Result<Vec<u8>> {
        if offset + len > self.size {
            return Err(MsiError::OutOfBounds {
                offset,
                len,
                size: self.size,
            });
        }

        let mut buf = vec![0u8; len];
        let fd = *self.fd.lock();

        let args = ffi::MsiStateRwRaw {
            handle_id: self.handle_id,
            offset: offset as u64,
            len: len as u64,
            buf_ptr: buf.as_mut_ptr() as u64,
        };

        unsafe {
            ffi::msi_ioc_state_read(fd, &mut { args })
                .map_err(|e| MsiError::Ioctl(format!("state_read: {}", e)))?;
        }

        Ok(buf)
    }

    /// Read the entire region.
    pub fn read_all(&self) -> Result<Vec<u8>> {
        self.read(0, self.size)
    }

    /// Write bytes to the region.
    pub fn write(&self, offset: usize, data: &[u8]) -> Result<()> {
        if self.perms != Perms::ReadWrite {
            return Err(MsiError::PermissionDenied(format!(
                "state '{}' is read-only", self.name
            )));
        }

        if offset + data.len() > self.size {
            return Err(MsiError::OutOfBounds {
                offset,
                len: data.len(),
                size: self.size,
            });
        }

        let fd = *self.fd.lock();

        let args = ffi::MsiStateRwRaw {
            handle_id: self.handle_id,
            offset: offset as u64,
            len: data.len() as u64,
            buf_ptr: data.as_ptr() as u64,
        };

        unsafe {
            ffi::msi_ioc_state_write(fd, &args)
                .map_err(|e| MsiError::Ioctl(format!("state_write: {}", e)))?;
        }

        Ok(())
    }

    /// Commit (flush) the region to persistent storage.
    pub fn commit(&self) -> Result<()> {
        let fd = *self.fd.lock();
        unsafe {
            ffi::msi_ioc_state_commit(fd, &self.handle_id)
                .map_err(|e| MsiError::Ioctl(format!("state_commit: {}", e)))?;
        }
        Ok(())
    }

    /// Write a serde-serializable value at offset 0.
    pub fn write_json<T: serde::Serialize>(&self, value: &T) -> Result<()> {
        let bytes = serde_json::to_vec(value)
            .map_err(|e| MsiError::Ioctl(format!("json serialize: {}", e)))?;
        self.write(0, &bytes)
    }

    /// Read and deserialize a JSON value from offset 0.
    pub fn read_json<T: serde::de::DeserializeOwned>(&self) -> Result<T> {
        let data = self.read_all()?;
        let end = data.iter().position(|&b| b == 0).unwrap_or(data.len());
        serde_json::from_slice(&data[..end])
            .map_err(|e| MsiError::Ioctl(format!("json deserialize: {}", e)))
    }
}
