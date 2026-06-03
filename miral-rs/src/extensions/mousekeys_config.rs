//! Mouse keys accessibility support.

use super::ServerExtension;

use std::pin::Pin;

/// Configures mouse keys support.
#[derive(Debug, Clone, Copy)]
pub struct MouseKeysConfig {
    enabled: bool,
}

impl MouseKeysConfig {
    /// Create mouse keys enabled by default.
    pub fn enabled() -> Self {
        Self { enabled: true }
    }

    /// Create mouse keys disabled by default.
    pub fn disabled() -> Self {
        Self { enabled: false }
    }
}

impl Default for MouseKeysConfig {
    fn default() -> Self {
        Self::enabled()
    }
}

impl ServerExtension for MouseKeysConfig {
    fn name(&self) -> &str {
        "MouseKeysConfig"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        miral_sys::ffi::miral_runner_add_mousekeys(runner, self.enabled);
    }
}
