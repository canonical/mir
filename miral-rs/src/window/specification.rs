//! Window specification builder for creating or modifying windows.

use crate::geometry::{Point, Size};
use crate::window::{
    DepthLayer, FocusMode, OrientationMode, PlacementGravity, PointerConfinementState, ShellChrome,
    Window, WindowState, WindowType,
};

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
    pub(crate) window_type: Option<WindowType>,
    pub(crate) name: Option<String>,
    pub(crate) parent: Option<Window>,
    pub(crate) min_width: Option<i32>,
    pub(crate) min_height: Option<i32>,
    pub(crate) max_width: Option<i32>,
    pub(crate) max_height: Option<i32>,
    pub(crate) depth_layer: Option<DepthLayer>,
    pub(crate) focus_mode: Option<FocusMode>,
    pub(crate) shell_chrome: Option<ShellChrome>,
    pub(crate) preferred_orientation: Option<OrientationMode>,
    pub(crate) confine_pointer: Option<PointerConfinementState>,
    pub(crate) attached_edges: Option<PlacementGravity>,
    pub(crate) visible_on_lock_screen: Option<bool>,
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

    /// Set the parent window.
    ///
    /// Child windows are positioned relative to their parent and move with it.
    pub fn with_parent(mut self, parent: &Window) -> Self {
        self.parent = Some(parent.clone());
        self
    }

    /// Set the minimum width.
    pub fn with_min_width(mut self, min_width: i32) -> Self {
        self.min_width = Some(min_width);
        self
    }

    /// Set the minimum height.
    pub fn with_min_height(mut self, min_height: i32) -> Self {
        self.min_height = Some(min_height);
        self
    }

    /// Set the maximum width.
    pub fn with_max_width(mut self, max_width: i32) -> Self {
        self.max_width = Some(max_width);
        self
    }

    /// Set the maximum height.
    pub fn with_max_height(mut self, max_height: i32) -> Self {
        self.max_height = Some(max_height);
        self
    }

    /// Set the depth layer.
    pub fn with_depth_layer(mut self, layer: DepthLayer) -> Self {
        self.depth_layer = Some(layer);
        self
    }

    /// Set the focus mode.
    pub fn with_focus_mode(mut self, mode: FocusMode) -> Self {
        self.focus_mode = Some(mode);
        self
    }

    /// Set the shell chrome mode.
    pub fn with_shell_chrome(mut self, chrome: ShellChrome) -> Self {
        self.shell_chrome = Some(chrome);
        self
    }

    /// Set the preferred orientation.
    pub fn with_preferred_orientation(mut self, orientation: OrientationMode) -> Self {
        self.preferred_orientation = Some(orientation);
        self
    }

    /// Set the pointer confinement state.
    pub fn with_confine_pointer(mut self, confinement: PointerConfinementState) -> Self {
        self.confine_pointer = Some(confinement);
        self
    }

    /// Set the attached edges for tiling/snapping.
    pub fn with_attached_edges(mut self, edges: PlacementGravity) -> Self {
        self.attached_edges = Some(edges);
        self
    }

    /// Set whether the window is visible on the lock screen.
    pub fn with_visible_on_lock_screen(mut self, visible: bool) -> Self {
        self.visible_on_lock_screen = Some(visible);
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

    /// Get the requested parent window, if set.
    pub fn parent(&self) -> Option<&Window> {
        self.parent.as_ref()
    }

    /// Get the requested depth layer, if set.
    pub fn depth_layer(&self) -> Option<DepthLayer> {
        self.depth_layer
    }

    /// Get the requested focus mode, if set.
    pub fn focus_mode(&self) -> Option<FocusMode> {
        self.focus_mode
    }

    /// Get the requested shell chrome, if set.
    pub fn shell_chrome(&self) -> Option<ShellChrome> {
        self.shell_chrome
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
            window_type: if data.has_type {
                Some(WindowType::from_raw(data.window_type))
            } else {
                None
            },
            name: if data.has_name {
                Some(data.name.clone())
            } else {
                None
            },
            parent: if data.has_parent && data.parent_id != 0 {
                Some(Window::from_ffi(
                    data.parent_id,
                    Point::default(),
                    Size::default(),
                ))
            } else {
                None
            },
            min_width: if data.has_min_width {
                Some(data.min_width)
            } else {
                None
            },
            min_height: if data.has_min_height {
                Some(data.min_height)
            } else {
                None
            },
            max_width: if data.has_max_width {
                Some(data.max_width)
            } else {
                None
            },
            max_height: if data.has_max_height {
                Some(data.max_height)
            } else {
                None
            },
            depth_layer: if data.has_depth_layer {
                Some(DepthLayer::from_raw(data.depth_layer))
            } else {
                None
            },
            focus_mode: if data.has_focus_mode {
                Some(FocusMode::from_raw(data.focus_mode))
            } else {
                None
            },
            shell_chrome: if data.has_shell_chrome {
                Some(ShellChrome::from_raw(data.shell_chrome))
            } else {
                None
            },
            preferred_orientation: if data.has_preferred_orientation {
                Some(OrientationMode::from_raw(data.preferred_orientation))
            } else {
                None
            },
            confine_pointer: if data.has_confine_pointer {
                Some(PointerConfinementState::from_raw(data.confine_pointer))
            } else {
                None
            },
            attached_edges: if data.has_attached_edges {
                Some(PlacementGravity(data.attached_edges))
            } else {
                None
            },
            visible_on_lock_screen: if data.has_visible_on_lock_screen {
                Some(data.visible_on_lock_screen)
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
            has_type: self.window_type.is_some(),
            window_type: self.window_type.map(|t| t.to_raw()).unwrap_or(0),
            has_name: self.name.is_some(),
            name: self.name.clone().unwrap_or_default(),
            has_parent: self.parent.is_some(),
            parent_id: self.parent.as_ref().map(|w| w.id()).unwrap_or(0),
            has_min_width: self.min_width.is_some(),
            min_width: self.min_width.unwrap_or(0),
            has_min_height: self.min_height.is_some(),
            min_height: self.min_height.unwrap_or(0),
            has_max_width: self.max_width.is_some(),
            max_width: self.max_width.unwrap_or(0),
            has_max_height: self.max_height.is_some(),
            max_height: self.max_height.unwrap_or(0),
            has_depth_layer: self.depth_layer.is_some(),
            depth_layer: self.depth_layer.map(|d| d.to_raw()).unwrap_or(0),
            has_focus_mode: self.focus_mode.is_some(),
            focus_mode: self.focus_mode.map(|f| f.to_raw()).unwrap_or(0),
            has_shell_chrome: self.shell_chrome.is_some(),
            shell_chrome: self.shell_chrome.map(|s| s.to_raw()).unwrap_or(0),
            has_preferred_orientation: self.preferred_orientation.is_some(),
            preferred_orientation: self.preferred_orientation.map(|o| o.to_raw()).unwrap_or(0),
            has_confine_pointer: self.confine_pointer.is_some(),
            confine_pointer: self.confine_pointer.map(|c| c.to_raw()).unwrap_or(0),
            has_attached_edges: self.attached_edges.is_some(),
            attached_edges: self.attached_edges.map(|e| e.0).unwrap_or(0),
            has_visible_on_lock_screen: self.visible_on_lock_screen.is_some(),
            visible_on_lock_screen: self.visible_on_lock_screen.unwrap_or(false),
        }
    }
}
