//! The Window handle type.

use crate::geometry::{Point, Size};

/// A handle to a window in the compositor.
///
/// Windows are lightweight, cloneable identifiers. They can be used as keys
/// in `HashMap` or `HashSet` for the external storage pattern.
///
/// A default-constructed `Window` is invalid (not backed by any surface).
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Window {
    id: u64,
    position: Point,
    size: Size,
}

impl Window {
    /// Create a window from its FFI components.
    pub(crate) fn from_ffi(id: u64, position: Point, size: Size) -> Self {
        Self { id, position, size }
    }

    /// Get the unique identifier for this window.
    ///
    /// This ID is stable for the lifetime of the window and can be used
    /// as a key in data structures.
    pub fn id(&self) -> u64 {
        self.id
    }

    /// Get the top-left position of the window frame.
    pub fn top_left(&self) -> Point {
        self.position
    }

    /// Get the size of the window frame (including decorations).
    pub fn size(&self) -> Size {
        self.size
    }

    /// Check if this window handle is valid (backed by a surface).
    pub fn is_valid(&self) -> bool {
        self.id != 0
    }
}

impl Default for Window {
    fn default() -> Self {
        Self {
            id: 0,
            position: Point::default(),
            size: Size::default(),
        }
    }
}
