//! Session lock listener for detecting when the session is locked or unlocked.

use super::ServerExtension;

use std::pin::Pin;

/// Listens for session lock and unlock events.
pub struct SessionLockListener {
    on_lock: Box<dyn Fn() + Send>,
    on_unlock: Box<dyn Fn() + Send>,
}

impl SessionLockListener {
    /// Create a new session lock listener with lock and unlock callbacks.
    pub fn new(on_lock: impl Fn() + Send + 'static, on_unlock: impl Fn() + Send + 'static) -> Self {
        Self {
            on_lock: Box::new(on_lock),
            on_unlock: Box::new(on_unlock),
        }
    }
}

impl ServerExtension for SessionLockListener {
    fn name(&self) -> &str {
        "SessionLockListener"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut mir_sys::ffi::MiralRunner>) {
        mir_sys::set_on_session_lock_callback(self.on_lock);
        mir_sys::set_on_session_unlock_callback(self.on_unlock);
        mir_sys::ffi::miral_runner_add_session_lock_listener(runner);
    }
}
