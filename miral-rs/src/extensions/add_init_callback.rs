//! Server init callback support.

use super::ServerExtension;

use std::pin::Pin;

/// Registers a callback that runs after the server initializes.
pub struct AddInitCallback {
    callback: Box<dyn FnOnce() + Send>,
}

impl AddInitCallback {
    /// Create a new init callback extension.
    pub fn new(callback: impl FnOnce() + Send + 'static) -> Self {
        Self {
            callback: Box::new(callback),
        }
    }
}

impl ServerExtension for AddInitCallback {
    fn name(&self) -> &str {
        "AddInitCallback"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        let Self { callback } = *self;
        miral_sys::set_on_init_callback(callback);
        miral_sys::ffi::miral_runner_add_init_callback(runner);
    }
}
