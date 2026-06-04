//! Window manager tools for performing actions from within a policy.

use std::pin::Pin;

use crate::geometry::{Displacement, Point, Rectangle, Size};
use crate::output::Zone;
use crate::window::{Window, WindowInfo, WindowSpecification, WindowState};

/// Provides actions that a policy can take to manage windows.
///
/// An instance of `WindowManagerTools` is provided when the policy is
/// constructed and should be stored in the policy struct.
///
/// All methods are safe to call from within policy callbacks. The tools
/// communicate with the compositor's internal state to effect changes.
///
/// # Safety
///
/// The raw pointer stored internally is guaranteed valid for the lifetime
/// of the policy (which is the lifetime of the server run). All tool
/// method calls happen within policy dispatch callbacks, so the pointer
/// is always valid when tools methods are invoked.
pub struct WindowManagerTools {
    /// Raw pointer to the C++ MiralTools object.
    /// Valid for the entire lifetime of the policy (guaranteed by the runner).
    raw: *mut mir_sys::ffi::MiralTools,
}

// Safety: MiralTools is only accessed from the compositor thread (single-threaded dispatch).
// The policy is Send (required by trait bounds) but tools are only used within dispatch.
unsafe impl Send for WindowManagerTools {}
unsafe impl Sync for WindowManagerTools {}

impl WindowManagerTools {
    /// Create a new WindowManagerTools from a raw pointer to the C++ MiralTools.
    ///
    /// # Safety
    /// The pointer must be valid for the lifetime of the policy.
    pub(crate) unsafe fn from_raw(ptr: *mut mir_sys::ffi::MiralTools) -> Self {
        Self { raw: ptr }
    }

    /// Create an uninitialized tools instance (pointer is null).
    ///
    /// The tools will be automatically initialized by the framework before
    /// any policy methods are called. Use this in your policy's `Default` impl.
    pub fn uninit() -> Self {
        Self {
            raw: std::ptr::null_mut(),
        }
    }

    /// Set the raw tools pointer (called during policy initialization).
    pub(crate) fn set_raw(&mut self, ptr: *mut mir_sys::ffi::MiralTools) {
        self.raw = ptr;
    }

    /// Get a pinned mutable reference to the underlying tools.
    fn pin_mut(&self) -> Pin<&mut mir_sys::ffi::MiralTools> {
        assert!(!self.raw.is_null(), "WindowManagerTools not initialized");
        unsafe { Pin::new_unchecked(&mut *self.raw) }
    }

    /// Get an immutable reference to the underlying tools.
    fn as_ref(&self) -> &mir_sys::ffi::MiralTools {
        assert!(!self.raw.is_null(), "WindowManagerTools not initialized");
        unsafe { &*self.raw }
    }

    /// Raise a window (and its children) to the top of the stacking order.
    ///
    /// This brings the window and all of its child/satellite windows to
    /// the top of the z-order.
    pub fn raise_tree(&self, window: &Window) {
        mir_sys::ffi::miral_tools_raise_tree_by_id(self.pin_mut(), window.id());
    }

    /// Set the input focus to the given window.
    ///
    /// The window will receive keyboard events after this call.
    pub fn select_active_window(&self, window: &Window) {
        mir_sys::ffi::miral_tools_select_active_window_by_id(self.pin_mut(), window.id());
    }

    /// Modify a window's properties according to the given specification.
    ///
    /// Only fields that are set in the specification will be changed.
    pub fn modify_window(&self, window: &Window, spec: &WindowSpecification) {
        let ffi_spec = spec.to_ffi();
        mir_sys::ffi::miral_tools_modify_window_by_id(self.pin_mut(), window.id(), &ffi_spec);
    }

    /// Focus the next application's window.
    pub fn focus_next_application(&self) {
        mir_sys::ffi::miral_tools_focus_next_application(self.pin_mut());
    }

    /// Focus the previous application's window.
    pub fn focus_prev_application(&self) {
        mir_sys::ffi::miral_tools_focus_prev_application(self.pin_mut());
    }

    /// Focus the next window within the active application.
    pub fn focus_next_within_application(&self) {
        mir_sys::ffi::miral_tools_focus_next_within_application(self.pin_mut());
    }

    /// Focus the previous window within the active application.
    pub fn focus_prev_within_application(&self) {
        mir_sys::ffi::miral_tools_focus_prev_within_application(self.pin_mut());
    }

    /// Ask a client to close its window gracefully.
    ///
    /// This sends a close request to the client (equivalent to clicking
    /// the window's close button). The client may choose to ignore it.
    pub fn ask_client_to_close(&self, window: &Window) {
        mir_sys::ffi::miral_tools_ask_client_to_close_by_id(self.pin_mut(), window.id());
    }

    /// Drag a window by the given displacement.
    pub fn drag_window(&self, window: &Window, movement: Displacement) {
        mir_sys::ffi::miral_tools_drag_window_by_id(self.pin_mut(), window.id(), movement.into());
    }

    /// Drag the currently active window by the given displacement.
    pub fn drag_active_window(&self, movement: Displacement) {
        mir_sys::ffi::miral_tools_drag_active_window(self.pin_mut(), movement.into());
    }

    /// Get the active (focused) window, if any.
    pub fn active_window(&self) -> Option<Window> {
        let id = mir_sys::ffi::miral_tools_active_window_id(self.as_ref());
        if id == 0 {
            None
        } else {
            Some(Window::from_ffi(id, Point::default(), Size::default()))
        }
    }

    /// Get the info for a specific window.
    ///
    /// Returns a snapshot of the window's current properties.
    pub fn info_for(&self, window: &Window) -> WindowInfo {
        let snapshot = mir_sys::ffi::miral_tools_info_for_window_id(self.as_ref(), window.id());
        WindowInfo::from_ffi(&snapshot, window.id())
    }

    /// Get the active application zone (the area available for tiling).
    ///
    /// The application zone is the output area minus any reserved space
    /// (e.g., panels, docks).
    pub fn active_application_zone(&self) -> Zone {
        let snapshot = mir_sys::ffi::miral_tools_active_application_zone(self.as_ref());
        Zone::from_ffi(&snapshot)
    }

    /// Get the rectangle of the active output.
    pub fn active_output(&self) -> Rectangle {
        mir_sys::ffi::miral_tools_active_output(self.as_ref()).into()
    }

    /// Get the number of connected applications.
    pub fn count_applications(&self) -> u32 {
        mir_sys::ffi::miral_tools_count_applications(self.as_ref())
    }

    /// Swap two windows in the stacking order.
    ///
    /// After this call, `window_a` will be at `window_b`'s old stacking position
    /// and vice versa.
    pub fn swap_tree_order(&self, window_a: &Window, window_b: &Window) {
        mir_sys::ffi::miral_tools_swap_tree_order_by_id(
            self.pin_mut(),
            window_a.id(),
            window_b.id(),
        );
    }

    /// Send a window tree to the back of the stacking order.
    pub fn send_tree_to_back(&self, window: &Window) {
        mir_sys::ffi::miral_tools_send_tree_to_back_by_id(self.pin_mut(), window.id());
    }

    /// Move the cursor to a specific point.
    pub fn move_cursor_to(&self, point: Point) {
        mir_sys::ffi::miral_tools_move_cursor_to(self.pin_mut(), point.into());
    }

    /// Find the window at a given point.
    ///
    /// Returns the topmost window under the point, or `None` if no window is there.
    pub fn window_at(&self, point: Point) -> Option<Window> {
        let id = mir_sys::ffi::miral_tools_window_id_at(self.as_ref(), point.into());
        if id == 0 {
            None
        } else {
            Some(Window::from_ffi(id, Point::default(), Size::default()))
        }
    }

    /// Get all window IDs in the active workspace.
    ///
    /// Returns a list of all windows, which can be used for iteration.
    pub fn all_windows(&self) -> Vec<Window> {
        let ids = mir_sys::ffi::miral_tools_all_window_ids(self.as_ref());
        ids.into_iter()
            .map(|id| Window::from_ffi(id, Point::default(), Size::default()))
            .collect()
    }

    /// Calculate the placement for a window given a new state.
    ///
    /// Returns the rectangle the window would occupy in the given state.
    pub fn place_and_size_for_state(
        &self,
        window: &Window,
        new_state: WindowState,
        current_rect: Rectangle,
    ) -> Rectangle {
        let ffi_rect: mir_sys::ffi::Rectangle = current_rect.into();
        mir_sys::ffi::miral_tools_place_and_size_for_state(
            self.as_ref(),
            window.id(),
            new_state.to_raw(),
            &ffi_rect,
        )
        .into()
    }
}

// WindowManagerTools is not Clone (raw pointer semantics) but can be Debug
impl std::fmt::Debug for WindowManagerTools {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("WindowManagerTools")
            .field("raw", &self.raw)
            .finish()
    }
}

impl Default for WindowManagerTools {
    fn default() -> Self {
        Self::uninit()
    }
}
