//! Window manager tools for performing actions from within a policy.

use std::pin::Pin;

use crate::geometry::{Point, Rectangle, Size};
use crate::window::{Window, WindowSpecification};

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
    raw: *mut miral_sys::ffi::MiralTools,
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
    pub(crate) unsafe fn from_raw(ptr: *mut miral_sys::ffi::MiralTools) -> Self {
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
    pub(crate) fn set_raw(&mut self, ptr: *mut miral_sys::ffi::MiralTools) {
        self.raw = ptr;
    }

    /// Get a pinned mutable reference to the underlying tools.
    fn pin_mut(&self) -> Pin<&mut miral_sys::ffi::MiralTools> {
        assert!(!self.raw.is_null(), "WindowManagerTools not initialized");
        unsafe { Pin::new_unchecked(&mut *self.raw) }
    }

    /// Get an immutable reference to the underlying tools.
    fn as_ref(&self) -> &miral_sys::ffi::MiralTools {
        assert!(!self.raw.is_null(), "WindowManagerTools not initialized");
        unsafe { &*self.raw }
    }

    /// Raise a window (and its children) to the top of the stacking order.
    ///
    /// This brings the window and all of its child/satellite windows to
    /// the top of the z-order.
    pub fn raise_tree(&self, window: &Window) {
        miral_sys::ffi::miral_tools_raise_tree_by_id(self.pin_mut(), window.id());
    }

    /// Set the input focus to the given window.
    ///
    /// The window will receive keyboard events after this call.
    pub fn select_active_window(&self, window: &Window) {
        miral_sys::ffi::miral_tools_select_active_window_by_id(self.pin_mut(), window.id());
    }

    /// Modify a window's properties according to the given specification.
    ///
    /// Only fields that are set in the specification will be changed.
    pub fn modify_window(&self, window: &Window, spec: &WindowSpecification) {
        let ffi_spec = spec.to_ffi();
        miral_sys::ffi::miral_tools_modify_window_by_id(self.pin_mut(), window.id(), &ffi_spec);
    }

    /// Focus the next application's window.
    pub fn focus_next_application(&self) {
        miral_sys::ffi::miral_tools_focus_next_application(self.pin_mut());
    }

    /// Focus the previous application's window.
    pub fn focus_prev_application(&self) {
        miral_sys::ffi::miral_tools_focus_prev_application(self.pin_mut());
    }

    /// Focus the next window within the active application.
    pub fn focus_next_within_application(&self) {
        miral_sys::ffi::miral_tools_focus_next_within_application(self.pin_mut());
    }

    /// Focus the previous window within the active application.
    pub fn focus_prev_within_application(&self) {
        miral_sys::ffi::miral_tools_focus_prev_within_application(self.pin_mut());
    }

    /// Ask a client to close its window gracefully.
    ///
    /// This sends a close request to the client (equivalent to clicking
    /// the window's close button). The client may choose to ignore it.
    pub fn ask_client_to_close(&self, window: &Window) {
        miral_sys::ffi::miral_tools_ask_client_to_close_by_id(self.pin_mut(), window.id());
    }

    /// Drag a window by the given displacement.
    pub fn drag_window(&self, window: &Window, movement: crate::geometry::Displacement) {
        miral_sys::ffi::miral_tools_drag_window_by_id(
            self.pin_mut(),
            window.id(),
            movement.into(),
        );
    }

    /// Get the active (focused) window, if any.
    pub fn active_window(&self) -> Option<Window> {
        let id = miral_sys::ffi::miral_tools_active_window_id(self.as_ref());
        if id == 0 {
            None
        } else {
            Some(Window::from_ffi(id, Point::default(), Size::default()))
        }
    }

    /// Get the active application zone (the area available for tiling).
    ///
    /// The application zone is the output area minus any reserved space
    /// (e.g., panels, docks).
    pub fn active_application_zone(&self) -> Rectangle {
        let zone = miral_sys::ffi::miral_tools_active_application_zone(self.as_ref());
        zone.extents.into()
    }

    /// Get the rectangle of the active output.
    pub fn active_output(&self) -> Rectangle {
        miral_sys::ffi::miral_tools_active_output(self.as_ref()).into()
    }

    /// Get the number of connected applications.
    pub fn count_applications(&self) -> u32 {
        miral_sys::ffi::miral_tools_count_applications(self.as_ref())
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
