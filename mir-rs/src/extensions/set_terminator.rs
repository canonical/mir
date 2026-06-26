//! Terminator callback support.

use super::ServerExtension;

use std::pin::Pin;

/// Registers a custom termination handler.
pub struct SetTerminator {
    handler: Box<dyn Fn(i32) + Send>,
}

impl SetTerminator {
    /// Create a new terminator handler extension.
    pub fn new(handler: impl Fn(i32) + Send + 'static) -> Self {
        Self {
            handler: Box::new(handler),
        }
    }
}

impl ServerExtension for SetTerminator {
    fn name(&self) -> &str {
        "SetTerminator"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut mir_sys::ffi::MiralRunner>) {
        let Self { handler } = *self;
        mir_sys::set_on_terminator_callback(handler);
        mir_sys::ffi::miral_runner_add_terminator(runner);
    }
}
