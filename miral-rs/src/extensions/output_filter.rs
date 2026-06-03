//! Output filter configuration.

use super::ServerExtension;

use std::pin::Pin;

/// Enables standard output filter command-line options.
#[derive(Debug, Default, Clone, Copy)]
pub struct OutputFilter;

impl OutputFilter {
    /// Create an output filter extension.
    pub fn new() -> Self {
        Self
    }
}

impl ServerExtension for OutputFilter {
    fn name(&self) -> &str {
        "OutputFilter"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        miral_sys::ffi::miral_runner_add_output_filter(runner);
    }
}
