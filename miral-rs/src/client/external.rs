//! External client launcher.

use crate::extensions::ServerExtension;

/// Launch external client applications as separate processes.
///
/// External clients connect to the compositor via the Wayland socket,
/// like any regular application.
///
/// Add this to [`MirRunner`](crate::runner::MirRunner) via `.add()` to register it
/// with the server. After the server starts, call [`launch()`](ExternalClientLauncher::launch)
/// to spawn processes with the correct Wayland environment.
#[derive(Debug, Clone, Default)]
pub struct ExternalClientLauncher;

impl ExternalClientLauncher {
    /// Create a new external client launcher.
    pub fn new() -> Self {
        Self
    }

    /// Launch an application by command string.
    ///
    /// Returns the pid of the launched process on success.
    ///
    /// # Precondition
    ///
    /// The server must have been started with this launcher registered via `.add()` on
    /// [`MirRunner`](crate::runner::MirRunner).
    pub fn launch(&self, command: &str) -> Result<i32, Box<dyn std::error::Error>> {
        let pid = miral_sys::ffi::miral_launcher_launch(command);
        if pid < 0 {
            Err(format!("ExternalClientLauncher::launch failed (pid={})", pid).into())
        } else {
            Ok(pid)
        }
    }
}

impl ServerExtension for ExternalClientLauncher {
    fn name(&self) -> &str {
        "ExternalClientLauncher"
    }

    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
}
