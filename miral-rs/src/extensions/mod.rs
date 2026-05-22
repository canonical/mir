//! Server extension types for adding functionality to the compositor.
//!
//! Extensions are added to [`MirRunner`](crate::runner::MirRunner) via the
//! builder pattern before starting the compositor.

/// A trait for types that configure or extend the Mir server.
///
/// Implement this trait for custom extensions that need to hook into
/// the server at startup time.
pub trait ServerExtension: Send + 'static {
    /// A human-readable name for this extension (used in logs).
    fn name(&self) -> &str;

    /// Downcast support for extracting configuration from extensions.
    fn as_any(&self) -> &dyn std::any::Any;
}

/// Configuration for Wayland protocol extensions.
///
/// Controls which Wayland protocol extensions are enabled in the compositor.
#[derive(Debug, Clone)]
pub struct WaylandExtensions {
    /// Extensions to enable (empty = enable defaults).
    pub(crate) enabled: Vec<String>,
}

impl WaylandExtensions {
    /// Create a default set of Wayland extensions.
    pub fn default() -> Self {
        Self {
            enabled: Vec::new(),
        }
    }

    /// Enable a specific Wayland extension by name.
    pub fn enable(mut self, extension: impl Into<String>) -> Self {
        self.enabled.push(extension.into());
        self
    }
}

impl ServerExtension for WaylandExtensions {
    fn name(&self) -> &str {
        "WaylandExtensions"
    }

    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
}

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
