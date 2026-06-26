//! Screen magnifier support.

use super::ServerExtension;

use crate::geometry::Size;
use std::pin::Pin;

/// A screen magnifier that renders a magnified region at the cursor position.
#[derive(Debug, Clone, Copy)]
pub struct Magnifier {
    enabled: bool,
    magnification: f32,
    capture_size: Size,
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

    /// Update whether the magnifier is enabled at runtime.
    ///
    /// # Precondition
    ///
    /// The server must have started with this extension registered via `.add()`.
    pub fn set_enabled(&self, enabled: bool) {
        mir_sys::ffi::miral_magnifier_set_enabled(enabled);
    }

    /// Update the magnification level at runtime.
    ///
    /// # Precondition
    ///
    /// The server must have started with this extension registered via `.add()`.
    pub fn set_magnification(&self, magnification: f32) {
        mir_sys::ffi::miral_magnifier_set_magnification(magnification);
    }

    /// Update the capture size at runtime.
    ///
    /// # Precondition
    ///
    /// The server must have started with this extension registered via `.add()`.
    pub fn set_capture_size(&self, size: Size) {
        mir_sys::ffi::miral_magnifier_set_capture_size(size.width, size.height);
    }
}

impl Default for Magnifier {
    fn default() -> Self {
        Self::new()
    }
}

impl ServerExtension for Magnifier {
    fn name(&self) -> &str {
        "Magnifier"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut mir_sys::ffi::MiralRunner>) {
        mir_sys::ffi::miral_runner_add_magnifier(
            runner,
            self.magnification,
            self.capture_size.width,
            self.capture_size.height,
            self.enabled,
        );
    }
}
