//! Bounce keys accessibility support.

use super::ServerExtension;

use std::pin::Pin;

/// Configures the bounce keys accessibility feature.
#[derive(Debug, Clone, Copy)]
pub struct BounceKeys {
    enabled: bool,
}

impl BounceKeys {
    /// Create bounce keys enabled by default.
    pub fn enabled() -> Self {
        Self { enabled: true }
    }

    /// Create bounce keys disabled by default.
    pub fn disabled() -> Self {
        Self { enabled: false }
    }
}

impl Default for BounceKeys {
    fn default() -> Self {
        Self::enabled()
    }
}

impl ServerExtension for BounceKeys {
    fn name(&self) -> &str {
        "BounceKeys"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        miral_sys::ffi::miral_runner_add_bounce_keys(runner, self.enabled);
    }
}
