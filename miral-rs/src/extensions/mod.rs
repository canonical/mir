//! Server extension types for adding functionality to the compositor.
//!
//! Extensions are added to [`MirRunner`](crate::runner::MirRunner) via the
//! builder pattern before starting the compositor.

pub mod decorations;
pub mod idle;
mod keymap;
pub mod magnifier;
pub mod session_lock;
mod traits;
mod wayland;
mod x11;

pub use decorations::Decorations;
pub use idle::IdleListener;
pub use keymap::Keymap;
pub use magnifier::Magnifier;
pub use session_lock::SessionLockListener;
pub use traits::ServerExtension;
pub use wayland::WaylandExtensions;
pub use x11::X11Support;
