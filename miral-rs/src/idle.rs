//! Idle state listener for detecting when the display dims, turns off, or wakes.
//!
//! Configure this on [`MirRunner`](crate::runner::MirRunner) with
//! [`MirRunner::idle_listener`](crate::runner::MirRunner::idle_listener).

/// Listens for display idle state changes.
#[derive(Default)]
pub struct IdleListener {
    pub(crate) on_dim: Option<Box<dyn Fn() + Send>>,
    pub(crate) on_off: Option<Box<dyn Fn() + Send>>,
    pub(crate) on_wake: Option<Box<dyn Fn() + Send>>,
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
