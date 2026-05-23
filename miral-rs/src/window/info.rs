//! Window information and metadata.

use crate::geometry::{Point, Rectangle, Size};
use crate::window::{Window, WindowState, WindowType};

/// Provides read-only information about a window.
///
/// Instances are provided by the compositor during policy callbacks.
/// Use [`WindowManagerTools`](crate::policy::WindowManagerTools) to modify windows.
#[derive(Debug, Clone)]
pub struct WindowInfo {
    window: Window,
    name: String,
    window_type: WindowType,
    state: WindowState,
    min_width: i32,
    min_height: i32,
    max_width: i32,
    max_height: i32,
    depth_layer: i32,
    has_parent: bool,
    can_be_active: bool,
    is_visible: bool,
}

impl WindowInfo {
    /// Create a WindowInfo from an FFI snapshot.
    pub(crate) fn from_ffi(snapshot: &miral_sys::ffi::WindowInfoSnapshot, window_id: u64) -> Self {
        Self {
            window: Window::from_ffi(window_id, snapshot.top_left.into(), snapshot.size.into()),
            name: snapshot.name.clone(),
            window_type: WindowType::from_raw(snapshot.window_type),
            state: WindowState::from_raw(snapshot.state),
            min_width: snapshot.min_width,
            min_height: snapshot.min_height,
            max_width: snapshot.max_width,
            max_height: snapshot.max_height,
            depth_layer: snapshot.depth_layer,
            has_parent: snapshot.has_parent,
            can_be_active: snapshot.can_be_active,
            is_visible: snapshot.is_visible,
        }
    }

    /// The window handle.
    pub fn window(&self) -> &Window {
        &self.window
    }

    /// The name of the window (often set by the client via `xdg_toplevel::set_title`).
    pub fn name(&self) -> &str {
        &self.name
    }

    /// The window type.
    pub fn window_type(&self) -> WindowType {
        self.window_type
    }

    /// The current window state.
    pub fn state(&self) -> WindowState {
        self.state
    }

    /// The top-left position of the window.
    pub fn position(&self) -> Point {
        self.window.top_left()
    }

    /// The size of the window.
    pub fn size(&self) -> Size {
        self.window.size()
    }

    /// The bounding rectangle of the window.
    pub fn rectangle(&self) -> Rectangle {
        Rectangle::new(self.window.top_left(), self.window.size())
    }

    /// The minimum width requested by the client.
    pub fn min_width(&self) -> i32 {
        self.min_width
    }

    /// The minimum height requested by the client.
    pub fn min_height(&self) -> i32 {
        self.min_height
    }

    /// The maximum width requested by the client.
    pub fn max_width(&self) -> i32 {
        self.max_width
    }

    /// The maximum height requested by the client.
    pub fn max_height(&self) -> i32 {
        self.max_height
    }

    /// The depth layer of the window.
    pub fn depth_layer(&self) -> i32 {
        self.depth_layer
    }

    /// Whether this window has a parent.
    pub fn has_parent(&self) -> bool {
        self.has_parent
    }

    /// Whether this window can receive focus.
    pub fn can_be_active(&self) -> bool {
        self.can_be_active
    }

    /// Whether this window is currently visible.
    pub fn is_visible(&self) -> bool {
        self.is_visible
    }
}
