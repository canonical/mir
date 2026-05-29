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
///
/// # Example
///
/// ```no_run
/// use miral::extensions::WaylandExtensions;
///
/// let extensions = WaylandExtensions::default()
///     .enable(WaylandExtensions::LAYER_SHELL)
///     .enable(WaylandExtensions::FOREIGN_TOPLEVEL_LIST);
/// ```
#[derive(Debug, Clone)]
pub struct WaylandExtensions {
    /// Extensions to enable (empty = enable defaults).
    pub(crate) enabled: Vec<String>,
    /// Extensions to explicitly disable.
    pub(crate) disabled: Vec<String>,
}

impl WaylandExtensions {
    // --- Well-known protocol extension names ---

    /// The `zwlr_layer_shell_v1` protocol for layer surfaces (panels, overlays).
    pub const LAYER_SHELL: &'static str = "zwlr_layer_shell_v1";
    /// The `zwlr_foreign_toplevel_management_v1` protocol for managing toplevels from other clients.
    pub const FOREIGN_TOPLEVEL_MANAGEMENT: &'static str = "zwlr_foreign_toplevel_management_v1";
    /// The `ext_foreign_toplevel_list_v1` protocol for listing toplevels.
    pub const FOREIGN_TOPLEVEL_LIST: &'static str = "ext_foreign_toplevel_list_v1";
    /// The `zwp_virtual_keyboard_manager_v1` protocol for virtual keyboards.
    pub const VIRTUAL_KEYBOARD: &'static str = "zwp_virtual_keyboard_manager_v1";
    /// The `zwp_input_method_manager_v2` protocol for input methods.
    pub const INPUT_METHOD: &'static str = "zwp_input_method_manager_v2";
    /// The `zwlr_screencopy_manager_v1` protocol for screen capture.
    pub const SCREENCOPY: &'static str = "zwlr_screencopy_manager_v1";
    /// The `ext_session_lock_manager_v1` protocol for session lock screens.
    pub const SESSION_LOCK: &'static str = "ext_session_lock_manager_v1";
    /// The `zwp_pointer_constraints_v1` protocol for constraining the pointer.
    pub const POINTER_CONSTRAINTS: &'static str = "zwp_pointer_constraints_v1";
    /// The `zwp_relative_pointer_manager_v1` protocol for relative pointer motion.
    pub const RELATIVE_POINTER: &'static str = "zwp_relative_pointer_manager_v1";
    /// The `wp_content_type_manager_v1` protocol for content type hints.
    pub const CONTENT_TYPE: &'static str = "wp_content_type_manager_v1";
    /// The `wp_idle_inhibit_manager_v1` protocol for preventing idle.
    pub const IDLE_INHIBIT: &'static str = "wp_idle_inhibit_manager_v1";

    /// Create a default set of Wayland extensions.
    pub fn default() -> Self {
        Self {
            enabled: Vec::new(),
            disabled: Vec::new(),
        }
    }

    /// Enable a specific Wayland extension by name.
    pub fn enable(mut self, extension: impl Into<String>) -> Self {
        self.enabled.push(extension.into());
        self
    }

    /// Disable a specific Wayland extension by name.
    ///
    /// This can be used to prevent an extension from being loaded even if
    /// it would otherwise be enabled by default.
    pub fn disable(mut self, extension: impl Into<String>) -> Self {
        self.disabled.push(extension.into());
        self
    }

    /// Check whether an extension is enabled.
    pub fn is_enabled(&self, extension: &str) -> bool {
        self.enabled.iter().any(|e| e == extension) && !self.disabled.iter().any(|e| e == extension)
    }

    /// Get the list of enabled extensions.
    pub fn enabled_extensions(&self) -> &[String] {
        &self.enabled
    }

    /// Get the list of disabled extensions.
    pub fn disabled_extensions(&self) -> &[String] {
        &self.disabled
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
