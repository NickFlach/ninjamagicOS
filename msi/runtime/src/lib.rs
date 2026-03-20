//! MSI v1.0 Userspace Runtime for ninjamagicOS
//!
//! This crate provides the Rust interface to the MSI kernel module
//! via /dev/msi ioctls. It is the foundation for all cognitive programs
//! running on ninjamagicOS, including the NinjaMagic Agent.
//!
//! # Architecture
//! ```text
//! ┌─────────────────────────────┐
//! │   Cognitive Program (Rust)  │
//! │   Agent / Skill / Service   │
//! ├─────────────────────────────┤
//! │   msi-runtime (this crate)  │
//! │   Safe Rust API             │
//! ├─────────────────────────────┤
//! │   /dev/msi ioctls (FFI)     │
//! ├─────────────────────────────┤
//! │   MSI Kernel Module (C)     │
//! └─────────────────────────────┘
//! ```

pub mod ffi;
pub mod domain;
pub mod lane;
pub mod event;
pub mod state;
pub mod assoc;
pub mod error;
pub mod substrate;

pub use error::{MsiError, Result};
pub use substrate::Substrate;
pub use domain::{Domain, DomainBuilder, Grant};
pub use lane::{Lane, LanePolicy, Priority, EnergyBudget, Affinity};
pub use event::{Event, EventBus, Subscription, QoS};
pub use state::{StateRegion, Perms};
pub use assoc::{AssocStore, AssocValue, AssocQuery, AssocResult};
