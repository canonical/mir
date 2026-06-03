//! X11/XWayland support configuration.

use super::ServerExtension;

use std::pin::Pin;

/// Configuration for X11 application support via XWayland.
#[derive(Debug, Clone)]
pub struct X11Support {
    enabled: bool,
}

impl X11Support {
    /// Enable X11 support.
    pub fn new() -> Self {
        Self { enabled: true }
    }

    /// Check if X11 support is enabled.
    pub fn is_enabled(&self) -> bool {
        self.enabled
    }
}

impl Default for X11Support {
    fn default() -> Self {
        Self::new()
    }
}

impl ServerExtension for X11Support {
    fn name(&self) -> &str {
        "X11Support"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        if self.enabled {
            miral_sys::ffi::miral_runner_add_x11_support(runner);
        }
    }
}
