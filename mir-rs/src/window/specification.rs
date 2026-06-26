//! Window specification builder for creating or modifying windows.

use crate::geometry::{Displacement, Point, Rectangle, Size};
use crate::window::{
    AspectRatio, DepthLayer, FocusMode, InputReceptionMode, OrientationMode, PlacementGravity,
    PlacementHints, PointerConfinementState, ShellChrome, TiledEdges, Window, WindowState,
    WindowType,
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
    pub(crate) output_id: Option<i32>,
    pub(crate) aux_rect: Option<Rectangle>,
    pub(crate) placement_hints: Option<PlacementHints>,
    pub(crate) window_placement_gravity: Option<PlacementGravity>,
    pub(crate) aux_rect_placement_gravity: Option<PlacementGravity>,
    pub(crate) aux_rect_placement_offset: Option<Displacement>,
    pub(crate) width_inc: Option<i32>,
    pub(crate) height_inc: Option<i32>,
    pub(crate) min_aspect: Option<AspectRatio>,
    pub(crate) max_aspect: Option<AspectRatio>,
    pub(crate) input_shape: Option<Vec<Rectangle>>,
    pub(crate) input_mode: Option<InputReceptionMode>,
    /// `Some(Some(rect))` = set to rect, `Some(None)` = clear, `None` = unchanged.
    pub(crate) exclusive_rect: Option<Option<Rectangle>>,
    pub(crate) ignore_exclusion_zones: Option<bool>,
    pub(crate) application_id: Option<String>,
    pub(crate) server_side_decorated: Option<bool>,
    pub(crate) tiled_edges: Option<TiledEdges>,
    pub(crate) alpha: Option<f32>,
    pub(crate) parent_size: Option<Size>,
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

    /// Set the output ID for fullscreen windows.
    pub fn with_output_id(mut self, id: i32) -> Self {
        self.output_id = Some(id);
        self
    }

    /// Set the auxiliary rectangle for relative positioning.
    pub fn with_aux_rect(mut self, rect: Rectangle) -> Self {
        self.aux_rect = Some(rect);
        self
    }

    /// Set the placement hints for popup positioning.
    pub fn with_placement_hints(mut self, hints: PlacementHints) -> Self {
        self.placement_hints = Some(hints);
        self
    }

    /// Set the window placement gravity.
    pub fn with_window_placement_gravity(mut self, gravity: PlacementGravity) -> Self {
        self.window_placement_gravity = Some(gravity);
        self
    }

    /// Set the auxiliary rectangle placement gravity.
    pub fn with_aux_rect_placement_gravity(mut self, gravity: PlacementGravity) -> Self {
        self.aux_rect_placement_gravity = Some(gravity);
        self
    }

    /// Set the auxiliary rectangle placement offset.
    pub fn with_aux_rect_placement_offset(mut self, offset: Displacement) -> Self {
        self.aux_rect_placement_offset = Some(offset);
        self
    }

    /// Set the width increment for resize snapping.
    pub fn with_width_inc(mut self, inc: i32) -> Self {
        self.width_inc = Some(inc);
        self
    }

    /// Set the height increment for resize snapping.
    pub fn with_height_inc(mut self, inc: i32) -> Self {
        self.height_inc = Some(inc);
        self
    }

    /// Set the minimum aspect ratio.
    pub fn with_min_aspect(mut self, aspect: AspectRatio) -> Self {
        self.min_aspect = Some(aspect);
        self
    }

    /// Set the maximum aspect ratio.
    pub fn with_max_aspect(mut self, aspect: AspectRatio) -> Self {
        self.max_aspect = Some(aspect);
        self
    }

    /// Set the input shape (list of rectangles defining the input region).
    pub fn with_input_shape(mut self, shape: Vec<Rectangle>) -> Self {
        self.input_shape = Some(shape);
        self
    }

    /// Set the input reception mode.
    pub fn with_input_mode(mut self, mode: InputReceptionMode) -> Self {
        self.input_mode = Some(mode);
        self
    }

    /// Set the exclusive rectangle (area not to be occluded).
    ///
    /// Pass `Some(rect)` to set an exclusive zone, or `None` to clear it.
    pub fn with_exclusive_rect(mut self, rect: Option<Rectangle>) -> Self {
        self.exclusive_rect = Some(rect);
        self
    }

    /// Set whether exclusion zones from other windows are ignored.
    pub fn with_ignore_exclusion_zones(mut self, ignore: bool) -> Self {
        self.ignore_exclusion_zones = Some(ignore);
        self
    }

    /// Set the application ID (D-bus service name / .desktop file basename).
    pub fn with_application_id(mut self, id: impl Into<String>) -> Self {
        self.application_id = Some(id.into());
        self
    }

    /// Set whether the window should have server-side decorations.
    pub fn with_server_side_decorated(mut self, decorated: bool) -> Self {
        self.server_side_decorated = Some(decorated);
        self
    }

    /// Set the tiled edges.
    pub fn with_tiled_edges(mut self, edges: TiledEdges) -> Self {
        self.tiled_edges = Some(edges);
        self
    }

    /// Set the window alpha (opacity), between 0.0 and 1.0.
    pub fn with_alpha(mut self, alpha: f32) -> Self {
        self.alpha = Some(alpha);
        self
    }

    /// Set the expected parent size for constrained popup placement.
    pub fn with_parent_size(mut self, size: Size) -> Self {
        self.parent_size = Some(size);
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
    pub(crate) fn from_ffi(data: &mir_sys::ffi::WindowSpecData) -> Self {
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
            output_id: if data.has_output_id {
                Some(data.output_id)
            } else {
                None
            },
            aux_rect: if data.has_aux_rect {
                Some(data.aux_rect.into())
            } else {
                None
            },
            placement_hints: if data.has_placement_hints {
                Some(PlacementHints(data.placement_hints))
            } else {
                None
            },
            window_placement_gravity: if data.has_window_placement_gravity {
                Some(PlacementGravity(data.window_placement_gravity))
            } else {
                None
            },
            aux_rect_placement_gravity: if data.has_aux_rect_placement_gravity {
                Some(PlacementGravity(data.aux_rect_placement_gravity))
            } else {
                None
            },
            aux_rect_placement_offset: if data.has_aux_rect_placement_offset {
                Some(data.aux_rect_placement_offset.into())
            } else {
                None
            },
            width_inc: if data.has_width_inc {
                Some(data.width_inc)
            } else {
                None
            },
            height_inc: if data.has_height_inc {
                Some(data.height_inc)
            } else {
                None
            },
            min_aspect: if data.has_min_aspect {
                Some(AspectRatio::new(
                    data.min_aspect_width,
                    data.min_aspect_height,
                ))
            } else {
                None
            },
            max_aspect: if data.has_max_aspect {
                Some(AspectRatio::new(
                    data.max_aspect_width,
                    data.max_aspect_height,
                ))
            } else {
                None
            },
            input_shape: if data.has_input_shape {
                Some(decode_input_shape(&data.input_shape))
            } else {
                None
            },
            input_mode: if data.has_input_mode {
                Some(InputReceptionMode::from_raw(data.input_mode))
            } else {
                None
            },
            exclusive_rect: if data.has_exclusive_rect {
                if data.exclusive_rect_is_set {
                    Some(Some(data.exclusive_rect.into()))
                } else {
                    Some(None)
                }
            } else {
                None
            },
            ignore_exclusion_zones: if data.has_ignore_exclusion_zones {
                Some(data.ignore_exclusion_zones)
            } else {
                None
            },
            application_id: if data.has_application_id {
                Some(data.application_id.clone())
            } else {
                None
            },
            server_side_decorated: if data.has_server_side_decorated {
                Some(data.server_side_decorated)
            } else {
                None
            },
            tiled_edges: if data.has_tiled_edges {
                Some(TiledEdges(data.tiled_edges))
            } else {
                None
            },
            alpha: if data.has_alpha {
                Some(data.alpha)
            } else {
                None
            },
            parent_size: if data.has_parent_size {
                Some(data.parent_size.into())
            } else {
                None
            },
        }
    }

    /// Convert to an FFI WindowSpecData.
    pub(crate) fn to_ffi(&self) -> mir_sys::ffi::WindowSpecData {
        mir_sys::ffi::WindowSpecData {
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
            has_output_id: self.output_id.is_some(),
            output_id: self.output_id.unwrap_or(0),
            has_aux_rect: self.aux_rect.is_some(),
            aux_rect: self.aux_rect.unwrap_or_default().into(),
            has_placement_hints: self.placement_hints.is_some(),
            placement_hints: self.placement_hints.map(|h| h.0).unwrap_or(0),
            has_window_placement_gravity: self.window_placement_gravity.is_some(),
            window_placement_gravity: self.window_placement_gravity.map(|g| g.0).unwrap_or(0),
            has_aux_rect_placement_gravity: self.aux_rect_placement_gravity.is_some(),
            aux_rect_placement_gravity: self.aux_rect_placement_gravity.map(|g| g.0).unwrap_or(0),
            has_aux_rect_placement_offset: self.aux_rect_placement_offset.is_some(),
            aux_rect_placement_offset: self.aux_rect_placement_offset.unwrap_or_default().into(),
            has_width_inc: self.width_inc.is_some(),
            width_inc: self.width_inc.unwrap_or(0),
            has_height_inc: self.height_inc.is_some(),
            height_inc: self.height_inc.unwrap_or(0),
            has_min_aspect: self.min_aspect.is_some(),
            min_aspect_width: self.min_aspect.map(|a| a.width).unwrap_or(0),
            min_aspect_height: self.min_aspect.map(|a| a.height).unwrap_or(0),
            has_max_aspect: self.max_aspect.is_some(),
            max_aspect_width: self.max_aspect.map(|a| a.width).unwrap_or(0),
            max_aspect_height: self.max_aspect.map(|a| a.height).unwrap_or(0),
            has_input_shape: self.input_shape.is_some(),
            input_shape: self
                .input_shape
                .as_ref()
                .map(|s| encode_input_shape(s))
                .unwrap_or_default(),
            has_input_mode: self.input_mode.is_some(),
            input_mode: self.input_mode.map(|m| m.to_raw()).unwrap_or(0),
            has_exclusive_rect: self.exclusive_rect.is_some(),
            exclusive_rect_is_set: self
                .exclusive_rect
                .as_ref()
                .map(|o| o.is_some())
                .unwrap_or(false),
            exclusive_rect: self
                .exclusive_rect
                .as_ref()
                .and_then(|o| *o)
                .unwrap_or_default()
                .into(),
            has_ignore_exclusion_zones: self.ignore_exclusion_zones.is_some(),
            ignore_exclusion_zones: self.ignore_exclusion_zones.unwrap_or(false),
            has_application_id: self.application_id.is_some(),
            application_id: self.application_id.clone().unwrap_or_default(),
            has_server_side_decorated: self.server_side_decorated.is_some(),
            server_side_decorated: self.server_side_decorated.unwrap_or(false),
            has_tiled_edges: self.tiled_edges.is_some(),
            tiled_edges: self.tiled_edges.map(|t| t.0).unwrap_or(0),
            has_alpha: self.alpha.is_some(),
            alpha: self.alpha.unwrap_or(0.0),
            has_parent_size: self.parent_size.is_some(),
            parent_size: self.parent_size.unwrap_or_default().into(),
        }
    }
}

/// Encode a list of rectangles as "x,y,w,h|x,y,w,h|..." for FFI transport.
fn encode_input_shape(rects: &[Rectangle]) -> String {
    rects
        .iter()
        .map(|r| {
            format!(
                "{},{},{},{}",
                r.top_left.x, r.top_left.y, r.size.width, r.size.height
            )
        })
        .collect::<Vec<_>>()
        .join("|")
}

/// Decode "x,y,w,h|x,y,w,h|..." back to a list of rectangles.
fn decode_input_shape(encoded: &str) -> Vec<Rectangle> {
    if encoded.is_empty() {
        return Vec::new();
    }
    encoded
        .split('|')
        .filter_map(|segment| {
            let parts: Vec<i32> = segment.split(',').filter_map(|s| s.parse().ok()).collect();
            if parts.len() == 4 {
                Some(Rectangle {
                    top_left: Point {
                        x: parts[0],
                        y: parts[1],
                    },
                    size: Size {
                        width: parts[2],
                        height: parts[3],
                    },
                })
            } else {
                None
            }
        })
        .collect()
}
