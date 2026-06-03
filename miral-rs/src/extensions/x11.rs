//! X11/XWayland support configuration.

use super::ServerExtension;

/// Configuration for X11 application support via XWayland.
#[derive(Debug, Clone)]
pub struct X11Support {
    enabled: bool,
}

impl X11Support {
    /// Enable X11 support.
    pub fn new() -> Self {
        Self { enabled: true }
    }

    /// Check if X11 support is enabled.
    pub fn is_enabled(&self) -> bool {
        self.enabled
    }
}

impl Default for X11Support {
    fn default() -> Self {
        Self::new()
    }
}

impl ServerExtension for X11Support {
    fn name(&self) -> &str {
        "X11Support"
    }

    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
}
