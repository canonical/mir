//! Server extension types for adding functionality to the compositor.
//!
//! Extensions are added to [`MirRunner`](crate::runner::MirRunner) via the
//! builder pattern before starting the compositor.

pub mod add_init_callback;
pub mod bounce_keys;
pub mod cursor_scale;
pub mod cursor_theme;
pub mod decorations;
pub mod display_configuration;
pub mod idle;
pub mod input_configuration;
mod keymap;
pub mod locate_pointer;
pub mod magnifier;
pub mod mousekeys_config;
pub mod output_filter;
pub mod session_lock;
pub mod set_terminator;
pub mod slow_keys;
pub mod sticky_keys;
mod traits;
mod wayland;
mod x11;

pub use add_init_callback::AddInitCallback;
pub use bounce_keys::BounceKeys;
pub use cursor_scale::CursorScale;
pub use cursor_theme::CursorTheme;
pub use decorations::Decorations;
pub use display_configuration::DisplayConfiguration;
pub use idle::IdleListener;
pub use input_configuration::InputConfiguration;
pub use keymap::Keymap;
pub use locate_pointer::LocatePointer;
pub use magnifier::Magnifier;
pub use mousekeys_config::MouseKeysConfig;
pub use output_filter::OutputFilter;
pub use session_lock::SessionLockListener;
pub use set_terminator::SetTerminator;
pub use slow_keys::SlowKeys;
pub use sticky_keys::StickyKeys;
pub use traits::ServerExtension;
pub use wayland::WaylandExtensions;
pub use x11::X11Support;
