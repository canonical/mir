//! Window types and state management.
//!
//! This module provides types for interacting with windows in the compositor,
//! including querying window properties, modifying window state, and building
//! window specifications.

mod info;
mod specification;
mod state;
mod window;

pub use info::WindowInfo;
pub use specification::WindowSpecification;
pub use state::{
    AspectRatio, DepthLayer, FocusMode, FocusStealing, InputReceptionMode, OrientationMode,
    PlacementGravity, PlacementHints, PointerConfinementState, ResizeEdge, ShellChrome, TiledEdges,
    WindowState, WindowType,
};
pub use window::Window;
