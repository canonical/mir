//! Server extension trait.

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
