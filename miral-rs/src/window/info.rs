//! Window information and metadata.

use crate::geometry::{Point, Rectangle, Size};
use crate::window::{
    AspectRatio, DepthLayer, FocusMode, OrientationMode, PlacementGravity,
    PointerConfinementState, ShellChrome, TiledEdges, Window, WindowState, WindowType,
};

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
    depth_layer: DepthLayer,
    has_parent: bool,
    parent_id: u64,
    can_be_active: bool,
    is_visible: bool,
    focus_mode: FocusMode,
    restore_rect: Rectangle,
    width_inc: i32,
    height_inc: i32,
    min_aspect: AspectRatio,
    max_aspect: AspectRatio,
    output_id: Option<i32>,
    preferred_orientation: OrientationMode,
    confine_pointer: PointerConfinementState,
    shell_chrome: ShellChrome,
    attached_edges: PlacementGravity,
    exclusive_rect: Option<Option<Rectangle>>,
    ignore_exclusion_zones: bool,
    clip_area: Option<Rectangle>,
    application_id: String,
    visible_on_lock_screen: bool,
    tiled_edges: TiledEdges,
    alpha: f32,
    must_have_parent: bool,
    must_not_have_parent: bool,
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
            depth_layer: DepthLayer::from_raw(snapshot.depth_layer),
            has_parent: snapshot.has_parent,
            parent_id: snapshot.parent_id,
            can_be_active: snapshot.can_be_active,
            is_visible: snapshot.is_visible,
            focus_mode: FocusMode::from_raw(snapshot.focus_mode),
            restore_rect: snapshot.restore_rect.into(),
            width_inc: snapshot.width_inc,
            height_inc: snapshot.height_inc,
            min_aspect: AspectRatio::new(
                snapshot.min_aspect_width as u32,
                snapshot.min_aspect_height as u32,
            ),
            max_aspect: AspectRatio::new(
                snapshot.max_aspect_width as u32,
                snapshot.max_aspect_height as u32,
            ),
            output_id: if snapshot.has_output_id {
                Some(snapshot.output_id)
            } else {
                None
            },
            preferred_orientation: OrientationMode::from_raw(snapshot.preferred_orientation),
            confine_pointer: PointerConfinementState::from_raw(snapshot.confine_pointer),
            shell_chrome: ShellChrome::from_raw(snapshot.shell_chrome),
            attached_edges: PlacementGravity(snapshot.attached_edges),
            exclusive_rect: if snapshot.has_exclusive_rect {
                if snapshot.has_exclusive_rect_value {
                    Some(Some(snapshot.exclusive_rect.into()))
                } else {
                    Some(None)
                }
            } else {
                None
            },
            ignore_exclusion_zones: snapshot.ignore_exclusion_zones,
            clip_area: if snapshot.has_clip_area {
                Some(snapshot.clip_area.into())
            } else {
                None
            },
            application_id: snapshot.application_id.clone(),
            visible_on_lock_screen: snapshot.visible_on_lock_screen,
            tiled_edges: TiledEdges(snapshot.tiled_edges),
            alpha: snapshot.alpha,
            must_have_parent: snapshot.must_have_parent,
            must_not_have_parent: snapshot.must_not_have_parent,
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
    pub fn depth_layer(&self) -> DepthLayer {
        self.depth_layer
    }

    /// Whether this window has a parent.
    pub fn has_parent(&self) -> bool {
        self.has_parent
    }

    /// The parent window's ID, if any.
    pub fn parent_id(&self) -> Option<u64> {
        if self.has_parent && self.parent_id != 0 {
            Some(self.parent_id)
        } else {
            None
        }
    }

    /// Whether this window can receive focus.
    pub fn can_be_active(&self) -> bool {
        self.can_be_active
    }

    /// Whether this window is currently visible.
    pub fn is_visible(&self) -> bool {
        self.is_visible
    }

    /// The focus mode for this window.
    pub fn focus_mode(&self) -> FocusMode {
        self.focus_mode
    }

    /// The restore rectangle (the bounds the window returns to from maximized/fullscreen).
    pub fn restore_rect(&self) -> Rectangle {
        self.restore_rect
    }

    /// The width increment for resize steps (0 means no constraint).
    pub fn width_inc(&self) -> i32 {
        self.width_inc
    }

    /// The height increment for resize steps (0 means no constraint).
    pub fn height_inc(&self) -> i32 {
        self.height_inc
    }

    /// The minimum aspect ratio.
    pub fn min_aspect(&self) -> AspectRatio {
        self.min_aspect
    }

    /// The maximum aspect ratio.
    pub fn max_aspect(&self) -> AspectRatio {
        self.max_aspect
    }

    /// The output ID this window is associated with, if any.
    pub fn output_id(&self) -> Option<i32> {
        self.output_id
    }

    /// The preferred orientation mode.
    pub fn preferred_orientation(&self) -> OrientationMode {
        self.preferred_orientation
    }

    /// The pointer confinement state.
    pub fn confine_pointer(&self) -> PointerConfinementState {
        self.confine_pointer
    }

    /// The shell chrome mode.
    pub fn shell_chrome(&self) -> ShellChrome {
        self.shell_chrome
    }

    /// Edges attached to screen edges (for tiling/snapping).
    pub fn attached_edges(&self) -> PlacementGravity {
        self.attached_edges
    }

    /// The exclusive zone rectangle.
    ///
    /// - `None` means the property is not set.
    /// - `Some(None)` means the exclusive zone has been explicitly cleared.
    /// - `Some(Some(rect))` means the exclusive zone is the given rectangle.
    pub fn exclusive_rect(&self) -> Option<Option<Rectangle>> {
        self.exclusive_rect
    }

    /// Whether this window ignores exclusion zones.
    pub fn ignore_exclusion_zones(&self) -> bool {
        self.ignore_exclusion_zones
    }

    /// The clip area, if set. Input outside this area is not delivered.
    pub fn clip_area(&self) -> Option<Rectangle> {
        self.clip_area
    }

    /// The application ID (e.g., desktop file ID).
    pub fn application_id(&self) -> &str {
        &self.application_id
    }

    /// Whether this window is visible on the lock screen.
    pub fn visible_on_lock_screen(&self) -> bool {
        self.visible_on_lock_screen
    }

    /// Edges touching the tiling grid.
    pub fn tiled_edges(&self) -> TiledEdges {
        self.tiled_edges
    }

    /// The opacity/alpha value (0.0 = fully transparent, 1.0 = fully opaque).
    pub fn alpha(&self) -> f32 {
        self.alpha
    }

    /// Whether this window type must have a parent.
    pub fn must_have_parent(&self) -> bool {
        self.must_have_parent
    }

    /// Whether this window type must not have a parent.
    pub fn must_not_have_parent(&self) -> bool {
        self.must_not_have_parent
    }
}
