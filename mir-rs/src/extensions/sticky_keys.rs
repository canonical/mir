//! Sticky keys accessibility support.

use super::ServerExtension;

use std::pin::Pin;

/// Configures the sticky keys accessibility feature.
#[derive(Debug, Clone, Copy)]
pub struct StickyKeys {
    enabled: bool,
}

impl StickyKeys {
    /// Create sticky keys enabled by default.
    pub fn enabled() -> Self {
        Self { enabled: true }
    }

    /// Create sticky keys disabled by default.
    pub fn disabled() -> Self {
        Self { enabled: false }
    }
}

impl Default for StickyKeys {
    fn default() -> Self {
        Self::enabled()
    }
}

impl ServerExtension for StickyKeys {
    fn name(&self) -> &str {
        "StickyKeys"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut mir_sys::ffi::MiralRunner>) {
        mir_sys::ffi::miral_runner_add_sticky_keys(runner, self.enabled);
    }
}
