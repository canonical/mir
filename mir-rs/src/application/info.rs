//! Application information and metadata.

use crate::application::Application;

/// Information about a connected client application.
///
/// Provides metadata such as the application name and a stable identifier.
#[derive(Debug, Clone)]
pub struct ApplicationInfo {
    application: Application,
    name: String,
}

impl ApplicationInfo {
    /// Create an ApplicationInfo from FFI data.
    pub(crate) fn from_ffi(app_id: u64, name: String) -> Self {
        Self {
            application: Application::from_id(app_id),
            name,
        }
    }

    /// Get the application handle (usable as a HashMap key).
    pub fn application(&self) -> &Application {
        &self.application
    }

    /// Get the name of the application.
    pub fn name(&self) -> &str {
        &self.name
    }
}
