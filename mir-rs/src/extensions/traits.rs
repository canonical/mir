//! Server extension trait.

use std::pin::Pin;

/// A trait for types that configure or extend the Mir server.
///
/// Implement this trait for custom extensions that need to hook into
/// the server at startup time. Each extension applies its own configuration
/// during [`MirRunner::run()`](crate::runner::MirRunner::run) by calling
/// the appropriate `mir_sys` FFI function to construct its C++ counterpart.
pub trait ServerExtension: Send + 'static {
    /// A human-readable name for this extension (used in logs).
    fn name(&self) -> &str;

    /// Apply this extension's configuration to the underlying C++ runner.
    ///
    /// Called during [`MirRunner::run()`](crate::runner::MirRunner::run) before
    /// the server starts. Each extension should call the appropriate
    /// `mir_sys::ffi::miral_runner_add_*` function to register itself.
    fn apply(self: Box<Self>, runner: Pin<&mut mir_sys::ffi::MiralRunner>);
}
