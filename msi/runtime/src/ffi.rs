//! FFI bindings to /dev/msi kernel module ioctls
//!
//! These map 1:1 to the structs and ioctl numbers defined in
//! kernel/drivers/msi/msi_core.h and msi_ioctl.c

use std::os::unix::io::RawFd;

// ===== ioctl magic and numbers =====

const MSI_IOC_MAGIC: u8 = b'M';

// Discovery
nix::ioctl_read!(msi_ioc_version, MSI_IOC_MAGIC, 0, u32);
nix::ioctl_read!(msi_ioc_capabilities, MSI_IOC_MAGIC, 1, MsiCapabilitiesRaw);

// Domains
nix::ioctl_readwrite!(msi_ioc_domain_create, MSI_IOC_MAGIC, 10, MsiDomainCreateRaw);
nix::ioctl_write_ptr!(msi_ioc_domain_grant, MSI_IOC_MAGIC, 11, MsiDomainGrantRaw);
nix::ioctl_write_ptr!(msi_ioc_domain_seal, MSI_IOC_MAGIC, 12, u32);

// Lanes
nix::ioctl_readwrite!(msi_ioc_lane_spawn, MSI_IOC_MAGIC, 20, MsiLaneSpawnRaw);
nix::ioctl_write_ptr!(msi_ioc_lane_yield, MSI_IOC_MAGIC, 21, u32);
nix::ioctl_write_ptr!(msi_ioc_lane_sleep, MSI_IOC_MAGIC, 22, u64);
nix::ioctl_write_ptr!(msi_ioc_lane_kill, MSI_IOC_MAGIC, 23, u32);

// Events
nix::ioctl_write_ptr!(msi_ioc_event_publish, MSI_IOC_MAGIC, 30, MsiEventPublishRaw);
nix::ioctl_readwrite!(msi_ioc_event_subscribe, MSI_IOC_MAGIC, 31, MsiEventSubscribeRaw);
nix::ioctl_readwrite!(msi_ioc_event_wait, MSI_IOC_MAGIC, 32, MsiEventWaitRaw);
nix::ioctl_write_ptr!(msi_ioc_event_ack, MSI_IOC_MAGIC, 33, u64);

// State
nix::ioctl_readwrite!(msi_ioc_state_map, MSI_IOC_MAGIC, 40, MsiStateMapRaw);
nix::ioctl_readwrite!(msi_ioc_state_read, MSI_IOC_MAGIC, 41, MsiStateRwRaw);
nix::ioctl_write_ptr!(msi_ioc_state_write, MSI_IOC_MAGIC, 42, MsiStateRwRaw);
nix::ioctl_write_ptr!(msi_ioc_state_commit, MSI_IOC_MAGIC, 43, u32);

// ===== Raw FFI structs (must match kernel layout exactly) =====

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct MsiCapabilitiesRaw {
    pub lanes_min: u32,
    pub lanes_max: u32,
    pub lanes_realtime: u32, // bool as u32 for C ABI
    pub events_max_topics: u32,
    pub state_max_bytes: u64,
    pub security_attest: u32,
    pub security_model: u32,
    pub accel_cpu: u32,
    pub accel_gpu: u32,
    pub accel_npu: u32,
    pub accel_dsp: u32,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MsiDomainCreateRaw {
    pub name: [u8; 128],
    pub num_grants: u32,
    pub seal: u32, // bool
    pub domain_id: u32, // out
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct MsiGrantRaw {
    pub kind: u32,
    pub data: [u8; 256], // union payload
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MsiDomainGrantRaw {
    pub domain_id: u32,
    pub grant: MsiGrantRaw,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct MsiLanePolicyRaw {
    pub priority: u32,
    pub energy: u32,
    pub affinity: u32,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MsiLaneSpawnRaw {
    pub domain_id: u32,
    pub entry: [u8; 128],
    pub policy: MsiLanePolicyRaw,
    pub lane_id: u32, // out
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MsiEventPublishRaw {
    pub domain_id: u32,
    pub topic: [u8; 256],
    pub payload_ptr: u64,
    pub payload_len: u32,
    pub qos: u32,
    pub event_id: u64, // out
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MsiEventSubscribeRaw {
    pub domain_id: u32,
    pub prefix: [u8; 256],
    pub filter: [u8; 256],
    pub sub_id: u32, // out
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MsiEventWaitRaw {
    pub sub_id: u32,
    pub timeout_nanos: u64,
    pub event_id: u64,
    pub topic: [u8; 256],
    pub ts_nanos: u64,
    pub payload_len: u32,
    pub payload_ptr: u64,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MsiStateMapRaw {
    pub domain_id: u32,
    pub name: [u8; 128],
    pub bytes: u64,
    pub perms: u32,
    pub handle_id: u32, // out
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MsiStateRwRaw {
    pub handle_id: u32,
    pub offset: u64,
    pub len: u64,
    pub buf_ptr: u64,
}

// ===== Helper: copy Rust string into fixed C buffer =====

pub fn str_to_buf<const N: usize>(s: &str) -> [u8; N] {
    let mut buf = [0u8; N];
    let bytes = s.as_bytes();
    let copy_len = bytes.len().min(N - 1);
    buf[..copy_len].copy_from_slice(&bytes[..copy_len]);
    buf
}

// ===== Helper: extract string from C buffer =====

pub fn buf_to_str(buf: &[u8]) -> String {
    let end = buf.iter().position(|&b| b == 0).unwrap_or(buf.len());
    String::from_utf8_lossy(&buf[..end]).to_string()
}

// ===== Device file handle =====

pub fn open_msi_device() -> std::io::Result<RawFd> {
    use std::os::unix::io::IntoRawFd;
    let file = std::fs::OpenOptions::new()
        .read(true)
        .write(true)
        .open("/dev/msi")?;
    Ok(file.into_raw_fd())
}
