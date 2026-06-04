//! Idle state listener for detecting when the display dims, turns off, or wakes.

use super::ServerExtension;

use std::pin::Pin;

/// Listens for display idle state changes.
#[derive(Default)]
pub struct IdleListener {
    on_dim: Option<Box<dyn Fn() + Send>>,
    on_off: Option<Box<dyn Fn() + Send>>,
    on_wake: Option<Box<dyn Fn() + Send>>,
}

impl IdleListener {
    /// Create a new idle listener with no callbacks.
    pub fn new() -> Self {
        Self::default()
    }

    /// Set a callback for when the display is about to dim.
    pub fn on_dim(mut self, f: impl Fn() + Send + 'static) -> Self {
        self.on_dim = Some(Box::new(f));
        self
    }

    /// Set a callback for when the display turns off.
    pub fn on_off(mut self, f: impl Fn() + Send + 'static) -> Self {
        self.on_off = Some(Box::new(f));
        self
    }

    /// Set a callback for when the display wakes from idle.
    pub fn on_wake(mut self, f: impl Fn() + Send + 'static) -> Self {
        self.on_wake = Some(Box::new(f));
        self
    }
}

impl ServerExtension for IdleListener {
    fn name(&self) -> &str {
        "IdleListener"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut mir_sys::ffi::MiralRunner>) {
        let has_callbacks =
            self.on_dim.is_some() || self.on_off.is_some() || self.on_wake.is_some();

        if let Some(callback) = self.on_dim {
            mir_sys::set_on_idle_dim_callback(callback);
        }
        if let Some(callback) = self.on_off {
            mir_sys::set_on_idle_off_callback(callback);
        }
        if let Some(callback) = self.on_wake {
            mir_sys::set_on_idle_wake_callback(callback);
        }
        if has_callbacks {
            mir_sys::ffi::miral_runner_add_idle_listener(runner);
        }
    }
}
