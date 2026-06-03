//! Display configuration support.

use super::ServerExtension;

use std::pin::Pin;

/// Enables loading display configuration from the runner's configured files.
#[derive(Debug, Default, Clone, Copy)]
pub struct DisplayConfiguration;

impl DisplayConfiguration {
    /// Create a display configuration extension.
    pub fn new() -> Self {
        Self
    }
}

impl ServerExtension for DisplayConfiguration {
    fn name(&self) -> &str {
        "DisplayConfiguration"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        miral_sys::ffi::miral_runner_add_display_configuration(runner);
    }
}
