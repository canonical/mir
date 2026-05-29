//! Session lock listener for detecting when the session is locked or unlocked.
//!
//! Configure this on [`MirRunner`](crate::runner::MirRunner) with
//! [`MirRunner::session_lock_listener`](crate::runner::MirRunner::session_lock_listener).

/// Listens for session lock and unlock events.
pub struct SessionLockListener {
    pub(crate) on_lock: Box<dyn Fn() + Send>,
    pub(crate) on_unlock: Box<dyn Fn() + Send>,
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
