//! Window types and state management.
//!
//! This module provides types for interacting with windows in the compositor,
//! including querying window properties, modifying window state, and building
//! window specifications.

mod window;
mod info;
mod specification;
mod state;

pub use window::Window;
pub use info::WindowInfo;
pub use specification::WindowSpecification;
pub use state::{WindowState, WindowType};
