//! Screen magnifier support.
//!
//! Configure this on [`MirRunner`](crate::runner::MirRunner) with
//! [`MirRunner::magnifier`](crate::runner::MirRunner::magnifier).

use crate::geometry::Size;

/// A screen magnifier that renders a magnified region at the cursor position.
#[derive(Debug, Clone, Copy)]
pub struct Magnifier {
    pub(crate) enabled: bool,
    pub(crate) magnification: f32,
    pub(crate) capture_size: Size,
}

impl Magnifier {
    /// Create a new magnifier with default settings.
    pub fn new() -> Self {
        Self {
            enabled: true,
            magnification: 2.0,
            capture_size: Size::new(400, 300),
        }
    }

    /// Set whether the magnifier is enabled.
    pub fn enabled(mut self, enabled: bool) -> Self {
        self.enabled = enabled;
        self
    }

    /// Set the magnification level.
    pub fn magnification(mut self, magnification: f32) -> Self {
        self.magnification = magnification;
        self
    }

    /// Set the capture size.
    pub fn capture_size(mut self, size: Size) -> Self {
        self.capture_size = size;
        self
    }
}

impl Default for Magnifier {
    fn default() -> Self {
        Self::new()
    }
}
