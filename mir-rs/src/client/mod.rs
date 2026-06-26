//! Client launching utilities for internal and external clients.
//!
//! These types allow the compositor to launch helper applications (e.g.,
//! panels, launchers) either as separate processes or within the compositor
//! process.

mod external;
mod internal;

pub use external::ExternalClientLauncher;
pub use internal::InternalClientLauncher;
