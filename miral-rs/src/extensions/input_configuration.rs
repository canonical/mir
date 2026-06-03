//! Input configuration support.

use super::ServerExtension;

use std::pin::Pin;

/// Enables standard input configuration command-line options.
#[derive(Debug, Default, Clone, Copy)]
pub struct InputConfiguration;

impl InputConfiguration {
    /// Create an input configuration extension.
    pub fn new() -> Self {
        Self
    }
}

impl ServerExtension for InputConfiguration {
    fn name(&self) -> &str {
        "InputConfiguration"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        miral_sys::ffi::miral_runner_add_input_configuration(runner);
    }
}
