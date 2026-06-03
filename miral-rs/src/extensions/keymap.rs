//! Keyboard layout (keymap) configuration.

use super::ServerExtension;

use std::pin::Pin;

/// Keyboard layout configuration.
///
/// Sets the XKB keymap for the compositor, controlling which keyboard
/// layout clients see.
///
/// # Example
///
/// ```rust
/// use miral::extensions::Keymap;
///
/// // US English layout
/// let us = Keymap::new("us");
///
/// // German layout with nodeadkeys variant
/// let de = Keymap::with_variant("de", "nodeadkeys");
/// ```
#[derive(Debug, Clone)]
pub struct Keymap {
    layout: String,
    variant: Option<String>,
}

impl Keymap {
    /// Create a keymap with the given XKB layout name.
    pub fn new(layout: impl Into<String>) -> Self {
        Self {
            layout: layout.into(),
            variant: None,
        }
    }

    /// Create a keymap with a layout and variant.
    pub fn with_variant(layout: impl Into<String>, variant: impl Into<String>) -> Self {
        Self {
            layout: layout.into(),
            variant: Some(variant.into()),
        }
    }

    /// Get the layout name.
    pub fn layout(&self) -> &str {
        &self.layout
    }

    /// Get the variant name, if set.
    pub fn variant(&self) -> Option<&str> {
        self.variant.as_deref()
    }
}

impl ServerExtension for Keymap {
    fn name(&self) -> &str {
        "Keymap"
    }

    fn apply(self: Box<Self>, runner: Pin<&mut miral_sys::ffi::MiralRunner>) {
        let layout = if let Some(variant) = &self.variant {
            format!("{}({})", self.layout, variant)
        } else {
            self.layout.clone()
        };
        miral_sys::ffi::miral_runner_add_keymap(runner, &layout);
    }
}
