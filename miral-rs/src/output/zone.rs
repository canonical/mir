//! Application zone type.

use crate::geometry::Rectangle;

/// An application zone — the usable area of an output for window tiling.
///
/// Zones represent the space available for application windows after
/// subtracting areas reserved for panels, docks, and other chrome.
/// When zones change (e.g., a panel is added/removed), the compositor
/// sends [`Advice::ZoneUpdate`](crate::policy::Advice::ZoneUpdate).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Zone {
    extents: Rectangle,
}

impl Zone {
    /// Create a zone from its extents.
    pub(crate) fn new(extents: Rectangle) -> Self {
        Self { extents }
    }

    /// The bounding rectangle of this zone.
    pub fn extents(&self) -> Rectangle {
        self.extents
    }
}
