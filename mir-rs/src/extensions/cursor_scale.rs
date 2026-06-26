//! Cursor scale configuration.

use super::ServerExtension;

use std::pin::Pin;

/// Configures the default cursor scale.
#[derive(Debug, Clone, Copy)]
pub struct CursorScale {
    scale: f32,
}

impl CursorScale {
    /// Create a cursor scale extension.
    pub fn new(scale: f32) -> Self {
        Self { scale }
    }

    /// Get the configured scale.
    pub fn scale(&self) -> f32 {
        self.scale
    }
}

impl Default for CursorScale {
    fn default() -> Self {
        Self::new(1.0)
    }
}

impl ServerExtension for CursorScale {
    fn name(&self) -> &str {
        "CursorScale"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut mir_sys::ffi::MiralRunner>) {
        mir_sys::ffi::miral_runner_add_cursor_scale(runner, self.scale);
    }
}
