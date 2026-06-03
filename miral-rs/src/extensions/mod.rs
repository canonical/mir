//! Server extension types for adding functionality to the compositor.
//!
//! Extensions are added to [`MirRunner`](crate::runner::MirRunner) via the
//! builder pattern before starting the compositor.

mod traits;
mod wayland;
mod x11;

pub use traits::ServerExtension;
pub use wayland::WaylandExtensions;
pub use x11::X11Support;
