//! Slow keys accessibility support.

use super::ServerExtension;

use std::pin::Pin;

/// Configures the slow keys accessibility feature.
#[derive(Debug, Clone, Copy)]
pub struct SlowKeys {
    enabled: bool,
}

impl SlowKeys {
    /// Create slow keys enabled by default.
    pub fn enabled() -> Self {
        Self { enabled: true }
    }

    /// Create slow keys disabled by default.
    pub fn disabled() -> Self {
        Self { enabled: false }
    }
}

impl Default for SlowKeys {
    fn default() -> Self {
        Self::enabled()
    }
}

impl ServerExtension for SlowKeys {
    fn name(&self) -> &str {
        "SlowKeys"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        miral_sys::ffi::miral_runner_add_slow_keys(runner, self.enabled);
    }
}
