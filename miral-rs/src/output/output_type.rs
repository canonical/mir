//! Display output type.

use crate::geometry::Rectangle;

/// Represents a physical display output.
///
/// Outputs correspond to monitors/screens connected to the system.
/// Each output has a unique ID, physical dimensions, and a logical
/// rectangle describing its position in the compositor's coordinate space.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Output {
    id: u32,
    extents: Rectangle,
    name: String,
}

impl Output {
    /// Create an output from FFI data.
    pub(crate) fn new(id: u32, extents: Rectangle, name: String) -> Self {
        Self { id, extents, name }
    }

    /// The unique identifier of this output.
    pub fn id(&self) -> u32 {
        self.id
    }

    /// The logical extents of this output in the compositor's coordinate space.
    pub fn extents(&self) -> Rectangle {
        self.extents
    }

    /// The name of this output (e.g., "HDMI-1", "eDP-1").
    pub fn name(&self) -> &str {
        &self.name
    }
}
