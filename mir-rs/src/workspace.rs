//! Workspace management.
//!
//! Workspaces are virtual desktops that group windows together.
//! Each workspace has a unique identifier and can be activated/deactivated.

/// A handle to a workspace (virtual desktop).
///
/// Workspaces are lightweight, cloneable identifiers. They can be used
/// as keys in `HashMap` or `HashSet`.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Workspace {
    id: u64,
}

impl Workspace {
    /// Create a workspace handle from a raw ID.
    pub(crate) fn from_id(id: u64) -> Self {
        Self { id }
    }

    /// Get the unique identifier for this workspace.
    pub fn id(&self) -> u64 {
        self.id
    }
}
