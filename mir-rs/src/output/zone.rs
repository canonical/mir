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
    id: i32,
    extents: Rectangle,
}

impl Zone {
    /// Create a zone from FFI data.
    pub(crate) fn from_ffi(snapshot: &mir_sys::ffi::ZoneSnapshot) -> Self {
        Self {
            id: snapshot.id,
            extents: snapshot.extents.into(),
        }
    }

    /// The unique identifier of this zone.
    pub fn id(&self) -> i32 {
        self.id
    }

    /// The bounding rectangle of this zone.
    pub fn extents(&self) -> Rectangle {
        self.extents
    }

    /// Set new extents (used when building modified zones).
    pub fn with_extents(mut self, extents: Rectangle) -> Self {
        self.extents = extents;
        self
    }

    /// Check if this is the same zone (by ID).
    pub fn is_same_zone(&self, other: &Zone) -> bool {
        self.id == other.id
    }
}
