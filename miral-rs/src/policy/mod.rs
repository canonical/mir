//! Window management policy and tools.
//!
//! The [`WindowManagementPolicy`] trait is the primary interface for compositor
//! authors. Implement it to define how windows are placed, moved, resized, and
//! otherwise managed.
//!
//! The [`WindowManagerTools`] struct provides actions the policy can take,
//! such as raising, focusing, or modifying windows.

mod traits;
mod tools;
pub(crate) mod adapter;

pub use traits::{Advice, WindowManagementPolicy};
pub use tools::WindowManagerTools;
