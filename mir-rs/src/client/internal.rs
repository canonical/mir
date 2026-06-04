//! Internal client launcher.

/// Launch internal client applications within the compositor process.
///
/// Internal clients share the compositor's process and can render
/// directly into compositor-owned surfaces (e.g., for wallpaper,
/// on-screen displays).
#[derive(Debug, Clone)]
pub struct InternalClientLauncher {
    _private: (),
}

impl InternalClientLauncher {
    /// Create a new internal client launcher.
    pub fn new() -> Self {
        Self { _private: () }
    }
}

impl Default for InternalClientLauncher {
    fn default() -> Self {
        Self::new()
    }
}
