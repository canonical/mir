//! Window specification builder for creating or modifying windows.

use crate::geometry::{Point, Size};
use crate::window::WindowState;

/// A specification describing desired window properties.
///
/// Use the builder methods to set properties, then pass to
/// [`WindowManagerTools::modify_window`](crate::policy::WindowManagerTools::modify_window).
///
/// Only set fields will be applied; unset fields leave the current value unchanged.
#[derive(Debug, Clone, Default)]
pub struct WindowSpecification {
    pub(crate) top_left: Option<Point>,
    pub(crate) size: Option<Size>,
    pub(crate) state: Option<WindowState>,
    pub(crate) name: Option<String>,
}

impl WindowSpecification {
    /// Create a new empty specification.
    pub fn new() -> Self {
        Self::default()
    }

    /// Set the top-left position of the window.
    pub fn with_top_left(mut self, top_left: Point) -> Self {
        self.top_left = Some(top_left);
        self
    }

    /// Set the size of the window.
    pub fn with_size(mut self, size: Size) -> Self {
        self.size = Some(size);
        self
    }

    /// Set the window state.
    pub fn with_state(mut self, state: WindowState) -> Self {
        self.state = Some(state);
        self
    }

    /// Set the window name/title.
    pub fn with_name(mut self, name: impl Into<String>) -> Self {
        self.name = Some(name.into());
        self
    }

    /// Get the requested top-left position, if set.
    pub fn top_left(&self) -> Option<Point> {
        self.top_left
    }

    /// Get the requested size, if set.
    pub fn size(&self) -> Option<Size> {
        self.size
    }

    /// Get the requested state, if set.
    pub fn state(&self) -> Option<WindowState> {
        self.state
    }

    /// Get the requested name, if set.
    pub fn name(&self) -> Option<&str> {
        self.name.as_deref()
    }

    /// Convert from an FFI WindowSpecData.
    pub(crate) fn from_ffi(data: &miral_sys::ffi::WindowSpecData) -> Self {
        Self {
            top_left: if data.has_top_left {
                Some(data.top_left.into())
            } else {
                None
            },
            size: if data.has_size {
                Some(data.size.into())
            } else {
                None
            },
            state: if data.has_state {
                Some(WindowState::from_raw(data.state))
            } else {
                None
            },
            name: if data.has_name {
                Some(data.name.clone())
            } else {
                None
            },
        }
    }

    /// Convert to an FFI WindowSpecData.
    pub(crate) fn to_ffi(&self) -> miral_sys::ffi::WindowSpecData {
        miral_sys::ffi::WindowSpecData {
            has_top_left: self.top_left.is_some(),
            top_left: self.top_left.unwrap_or_default().into(),
            has_size: self.size.is_some(),
            size: self.size.unwrap_or_default().into(),
            has_state: self.state.is_some(),
            state: self.state.map(|s| s.to_raw()).unwrap_or(0),
            has_type: false,
            window_type: 0,
            has_name: self.name.is_some(),
            name: self.name.clone().unwrap_or_default(),
        }
    }
}
