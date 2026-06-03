//! Cursor theme configuration.

use super::ServerExtension;

use std::pin::Pin;

/// Loads a cursor theme by name.
#[derive(Debug, Clone)]
pub struct CursorTheme {
    theme: String,
}

impl CursorTheme {
    /// Create a cursor theme extension.
    pub fn new(theme: impl Into<String>) -> Self {
        Self { theme: theme.into() }
    }

    /// Get the configured theme list.
    pub fn theme(&self) -> &str {
        &self.theme
    }
}

impl ServerExtension for CursorTheme {
    fn name(&self) -> &str {
        "CursorTheme"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        miral_sys::ffi::miral_runner_add_cursor_theme(runner, &self.theme);
    }
}
