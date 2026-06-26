//! Application handle type.

/// A handle to a connected client application.
///
/// Applications are lightweight, cloneable identifiers suitable for use
/// as keys in `HashMap` or `HashSet`.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Application {
    id: u64,
}

impl Application {
    /// Create an application from its FFI identifier.
    pub(crate) fn from_id(id: u64) -> Self {
        Self { id }
    }

    /// Get the unique identifier for this application.
    pub fn id(&self) -> u64 {
        self.id
    }
}
