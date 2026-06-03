//! Locate pointer accessibility support.

use super::ServerExtension;

use std::pin::Pin;

/// Configures the locate pointer accessibility feature.
#[derive(Debug, Clone, Copy)]
pub struct LocatePointer {
    enabled: bool,
}

impl LocatePointer {
    /// Create locate pointer enabled by default.
    pub fn enabled() -> Self {
        Self { enabled: true }
    }

    /// Create locate pointer disabled by default.
    pub fn disabled() -> Self {
        Self { enabled: false }
    }
}

impl Default for LocatePointer {
    fn default() -> Self {
        Self::enabled()
    }
}

impl ServerExtension for LocatePointer {
    fn name(&self) -> &str {
        "LocatePointer"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        miral_sys::ffi::miral_runner_add_locate_pointer(runner, self.enabled);
    }
}
