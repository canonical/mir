//! The WindowManagementPolicy trait and Advice enum.

use crate::application::ApplicationInfo;
use crate::geometry::{Displacement, Point, Rectangle, Size};
use crate::input::{InputEvent, KeyboardEvent, PointerEvent, TouchEvent};
use crate::output::{Output, Zone};
use crate::policy::WindowManagerTools;
use crate::window::{Window, WindowInfo, WindowSpecification, WindowState};

/// Advisory notifications from the compositor.
///
/// These are fire-and-forget notifications — no response is expected.
/// Use `match` to handle the events you care about and ignore the rest
/// with a wildcard (`_ => {}`).
///
/// Marked `#[non_exhaustive]` so new events can be added without breaking
/// existing code.
#[non_exhaustive]
pub enum Advice {
    /// A group of related notifications is starting.
    Begin,
    /// A group of related notifications has ended.
    End,

    // --- Application lifecycle ---
    /// A new application has connected.
    NewApp {
        /// The application info for the newly connected app.
        app: ApplicationInfo,
    },
    /// An application has disconnected.
    DeleteApp {
        /// The application info for the disconnecting app.
        app: ApplicationInfo,
    },

    // --- Window lifecycle ---
    /// A new window has been created.
    NewWindow {
        /// Information about the new window.
        window_info: WindowInfo,
    },
    /// A window is being destroyed.
    DeleteWindow {
        /// Information about the window being destroyed.
        window_info: WindowInfo,
    },
    /// A window has gained input focus.
    FocusGained {
        /// Information about the focused window.
        window_info: WindowInfo,
    },
    /// A window has lost input focus.
    FocusLost {
        /// Information about the window that lost focus.
        window_info: WindowInfo,
    },
    /// A window's state has changed (e.g., maximized, minimized).
    StateChange {
        /// Information about the window whose state changed.
        window_info: WindowInfo,
        /// The new state.
        state: WindowState,
    },
    /// A window has moved to a new position.
    MoveTo {
        /// Information about the moved window.
        window_info: WindowInfo,
        /// The new top-left position.
        top_left: Point,
    },
    /// A window has been resized.
    Resize {
        /// Information about the resized window.
        window_info: WindowInfo,
        /// The new size.
        new_size: Size,
    },
    /// Windows have been raised in the stacking order.
    Raise {
        /// The windows that were raised.
        windows: Vec<Window>,
    },

    // --- Output lifecycle ---
    /// A new output has been connected.
    OutputCreate {
        /// The new output.
        output: Output,
    },
    /// An existing output's properties have changed.
    OutputUpdate {
        /// The updated output state.
        updated: Output,
        /// The previous output state.
        original: Output,
    },
    /// An output has been disconnected.
    OutputDelete {
        /// The disconnected output.
        output: Output,
    },

    // --- Application zone lifecycle ---
    /// A new application zone has been created.
    ZoneCreate {
        /// The new zone.
        zone: Zone,
    },
    /// An existing zone's extents have changed.
    ZoneUpdate {
        /// The updated zone.
        updated: Zone,
        /// The previous zone extents.
        original: Zone,
    },
    /// A zone has been removed.
    ZoneDelete {
        /// The removed zone.
        zone: Zone,
    },
}

/// The primary trait for implementing a window management policy.
///
/// All methods have sensible defaults — a policy that only implements `tools()`
/// will behave like a floating window manager that honors all client requests.
///
/// Override individual methods to customize behavior. The `advise` method
/// receives lifecycle notifications that don't require a response.
///
/// # Example
///
/// ```rust,ignore
/// struct MyPolicy {
///     tools: WindowManagerTools,
/// }
///
/// impl WindowManagementPolicy for MyPolicy {
///     fn tools(&self) -> &WindowManagerTools { &self.tools }
///
///     fn place_new_window(
///         &mut self,
///         _app_info: &ApplicationInfo,
///         requested: &WindowSpecification,
///     ) -> WindowSpecification {
///         // Place all windows at (0, 0)
///         requested.clone().with_top_left(Point::new(0, 0))
///     }
/// }
/// ```
pub trait WindowManagementPolicy: Send + 'static {
    /// Access the window manager tools (immutable).
    ///
    /// This is the only required method. The tools provide actions like
    /// raising windows, setting focus, and modifying window properties.
    fn tools(&self) -> &WindowManagerTools;

    /// Access the window manager tools (mutable).
    ///
    /// Used internally by the framework to initialize the tools pointer.
    /// Override only if your tools field has a non-standard name.
    fn tools_mut(&mut self) -> &mut WindowManagerTools;

    /// Called to determine where a new window should be placed.
    ///
    /// Default: honors the requested specification as-is.
    fn place_new_window(
        &mut self,
        _app_info: &ApplicationInfo,
        requested: &WindowSpecification,
    ) -> WindowSpecification {
        requested.clone()
    }

    /// Called when a window's first buffer has been posted and it is ready to display.
    ///
    /// Default: no-op.
    fn handle_window_ready(&mut self, _window_info: &WindowInfo) {}

    /// Called when a client requests modifications to its window.
    ///
    /// Default: applies all requested modifications.
    fn handle_modify_window(
        &mut self,
        window_info: &WindowInfo,
        modifications: &WindowSpecification,
    ) {
        self.tools().modify_window(window_info.window(), modifications);
    }

    /// Called when a client requests its window be raised.
    ///
    /// Default: raises the window tree.
    fn handle_raise_window(&mut self, window_info: &WindowInfo) {
        self.tools().raise_tree(window_info.window());
    }

    /// Confirm placement of a maximized/fullscreen window.
    ///
    /// Default: accepts the suggested placement rectangle.
    fn confirm_placement_on_display(
        &mut self,
        _window_info: &WindowInfo,
        _new_state: WindowState,
        new_placement: Rectangle,
    ) -> Rectangle {
        new_placement
    }

    /// Handle a keyboard event. Return `true` if consumed.
    ///
    /// Default: not consumed (passes through to clients).
    fn handle_keyboard_event(&mut self, _event: &KeyboardEvent) -> bool {
        false
    }

    /// Handle a touch event. Return `true` if consumed.
    ///
    /// Default: not consumed (passes through to clients).
    fn handle_touch_event(&mut self, _event: &TouchEvent) -> bool {
        false
    }

    /// Handle a pointer (mouse) event. Return `true` if consumed.
    ///
    /// Default: not consumed (passes through to clients).
    fn handle_pointer_event(&mut self, _event: &PointerEvent) -> bool {
        false
    }

    /// Handle a client-initiated interactive move request.
    ///
    /// This is triggered by the client calling `xdg_toplevel::move`.
    /// Default: no-op.
    fn handle_request_move(&mut self, _window_info: &WindowInfo, _input_event: &InputEvent) {}

    /// Handle a client-initiated interactive resize request.
    ///
    /// This is triggered by the client calling `xdg_toplevel::resize`.
    /// Default: no-op.
    fn handle_request_resize(
        &mut self,
        _window_info: &WindowInfo,
        _input_event: &InputEvent,
        _edge: u32,
    ) {
    }

    /// Confirm child window placement when its parent moves.
    ///
    /// Default: applies the displacement to the current position.
    fn confirm_inherited_move(
        &mut self,
        window_info: &WindowInfo,
        movement: Displacement,
    ) -> Rectangle {
        Rectangle {
            top_left: window_info.window().top_left() + movement,
            size: window_info.window().size(),
        }
    }

    /// Called for all advisory notifications from the compositor.
    ///
    /// Override this to react to lifecycle events such as new/deleted
    /// applications, windows, outputs, and zones.
    ///
    /// Default: no-op.
    ///
    /// # Example
    ///
    /// ```rust,ignore
    /// fn advise(&mut self, event: Advice) {
    ///     match event {
    ///         Advice::NewApp { app } => {
    ///             println!("New app: {:?}", app.name());
    ///         }
    ///         Advice::ZoneUpdate { updated, .. } => {
    ///             self.retile(updated.extents());
    ///         }
    ///         _ => {}
    ///     }
    /// }
    /// ```
    fn advise(&mut self, _event: Advice) {}
}
