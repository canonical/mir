/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//! Low-level FFI bindings to the miral C++ library.
//!
//! This crate provides:
//! - **bindgen-generated** Rust equivalents of C-linkage Mir enums
//! - **cxx.rs bridge** for C++ class interactions (MirRunner, WindowManagerTools, etc.)
//!
//! Compositor authors should use the `mir` crate instead of this one directly.

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

/// Raw bindgen-generated bindings for Mir C enums.
///
/// These are auto-generated from `mir_toolkit/common.h` and `mir_toolkit/events/enums.h`.
pub mod enums {
    include!(concat!(env!("OUT_DIR"), "/mir_enums.rs"));
}

#[cxx::bridge(namespace = "mir_sys")]
pub mod ffi {
    /// Geometry types shared between Rust and C++.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    struct Point {
        x: i32,
        y: i32,
    }

    /// A 2D size with width and height.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    struct Size {
        width: i32,
        height: i32,
    }

    /// A rectangle defined by a top-left point and a size.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    struct Rectangle {
        top_left: Point,
        size: Size,
    }

    /// A 2D displacement vector.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    struct Displacement {
        dx: i32,
        dy: i32,
    }

    /// Information about a keyboard event, extracted on the C++ side.
    #[derive(Debug, Clone, Copy)]
    struct KeyboardEventInfo {
        action: i32,
        keysym: i32,
        scan_code: i32,
        modifiers: u32,
        event_time: i64,
    }

    /// Information about a pointer event, extracted on the C++ side.
    #[derive(Debug, Clone, Copy)]
    struct PointerEventInfo {
        action: i32,
        modifiers: u32,
        buttons: u32,
        x: f32,
        y: f32,
        dx: f32,
        dy: f32,
        event_time: i64,
    }

    /// Information about a touch event, extracted on the C++ side.
    #[derive(Debug, Clone, Copy)]
    struct TouchEventInfo {
        id: i32,
        action: i32,
        modifiers: u32,
        x: f32,
        y: f32,
        event_time: i64,
    }

    /// Read-only snapshot of window information passed across FFI.
    #[derive(Debug, Clone)]
    struct WindowInfoSnapshot {
        name: String,
        window_type: i32,
        state: i32,
        top_left: Point,
        size: Size,
        min_width: i32,
        min_height: i32,
        max_width: i32,
        max_height: i32,
        depth_layer: i32,
        has_parent: bool,
        can_be_active: bool,
        is_visible: bool,
        focus_mode: i32,
        restore_rect: Rectangle,
        width_inc: i32,
        height_inc: i32,
        min_aspect_width: i32,
        min_aspect_height: i32,
        max_aspect_width: i32,
        max_aspect_height: i32,
        has_output_id: bool,
        output_id: i32,
        preferred_orientation: i32,
        confine_pointer: i32,
        shell_chrome: i32,
        attached_edges: u32,
        has_exclusive_rect: bool,
        has_exclusive_rect_value: bool,
        exclusive_rect: Rectangle,
        ignore_exclusion_zones: bool,
        has_clip_area: bool,
        clip_area: Rectangle,
        application_id: String,
        visible_on_lock_screen: bool,
        tiled_edges: u32,
        alpha: f32,
        parent_id: u64,
        must_have_parent: bool,
        must_not_have_parent: bool,
    }

    /// Read-only snapshot of application information passed across FFI.
    #[derive(Debug, Clone)]
    struct ApplicationInfoSnapshot {
        name: String,
        window_count: u32,
    }

    /// Read-only snapshot of output information passed across FFI.
    #[derive(Debug, Clone)]
    struct OutputSnapshot {
        id: i32,
        name: String,
        connected: bool,
        used: bool,
        extents: Rectangle,
        refresh_rate: f64,
        scale: f32,
        power_mode: i32,
        orientation: i32,
        form_factor: i32,
        output_type: i32,
        physical_width_mm: i32,
        physical_height_mm: i32,
    }

    /// Read-only snapshot of zone information passed across FFI.
    #[derive(Debug, Clone, Copy)]
    struct ZoneSnapshot {
        id: i32,
        extents: Rectangle,
    }

    /// Fields for a window specification, used to create or modify windows.
    #[derive(Debug, Clone)]
    struct WindowSpecData {
        has_top_left: bool,
        top_left: Point,
        has_size: bool,
        size: Size,
        has_state: bool,
        state: i32,
        has_type: bool,
        window_type: i32,
        has_name: bool,
        name: String,
        has_parent: bool,
        parent_id: u64,
        has_min_width: bool,
        min_width: i32,
        has_min_height: bool,
        min_height: i32,
        has_max_width: bool,
        max_width: i32,
        has_max_height: bool,
        max_height: i32,
        has_depth_layer: bool,
        depth_layer: i32,
        has_focus_mode: bool,
        focus_mode: i32,
        has_shell_chrome: bool,
        shell_chrome: i32,
        has_preferred_orientation: bool,
        preferred_orientation: i32,
        has_confine_pointer: bool,
        confine_pointer: i32,
        has_attached_edges: bool,
        attached_edges: u32,
        has_visible_on_lock_screen: bool,
        visible_on_lock_screen: bool,
        has_output_id: bool,
        output_id: i32,
        has_aux_rect: bool,
        aux_rect: Rectangle,
        has_placement_hints: bool,
        placement_hints: u32,
        has_window_placement_gravity: bool,
        window_placement_gravity: u32,
        has_aux_rect_placement_gravity: bool,
        aux_rect_placement_gravity: u32,
        has_aux_rect_placement_offset: bool,
        aux_rect_placement_offset: Displacement,
        has_width_inc: bool,
        width_inc: i32,
        has_height_inc: bool,
        height_inc: i32,
        has_min_aspect: bool,
        min_aspect_width: u32,
        min_aspect_height: u32,
        has_max_aspect: bool,
        max_aspect_width: u32,
        max_aspect_height: u32,
        /// Encoded as "x,y,w,h|x,y,w,h|..." or empty string if unset.
        has_input_shape: bool,
        input_shape: String,
        has_input_mode: bool,
        input_mode: i32,
        /// Tri-state: has_exclusive_rect = outer is_set,
        /// exclusive_rect_is_set = inner is_set, exclusive_rect = value.
        has_exclusive_rect: bool,
        exclusive_rect_is_set: bool,
        exclusive_rect: Rectangle,
        has_ignore_exclusion_zones: bool,
        ignore_exclusion_zones: bool,
        has_application_id: bool,
        application_id: String,
        has_server_side_decorated: bool,
        server_side_decorated: bool,
        has_tiled_edges: bool,
        tiled_edges: u32,
        has_alpha: bool,
        alpha: f32,
        has_parent_size: bool,
        parent_size: Size,
    }

    /// Descriptor for a configuration option, passed from Rust to C++.
    ///
    /// The `option_type` field determines which callback variant is used:
    ///   0 = int (with default), 1 = double (with default), 2 = string (with default),
    ///   3 = bool (with default), 4 = flag (presence-only), 5 = optional int,
    ///   6 = optional string, 7 = optional bool, 8 = multi-value string list
    #[derive(Debug, Clone)]
    struct ConfigOptionDesc {
        name: String,
        description: String,
        option_type: i32,
        default_int: i32,
        default_double: f64,
        default_string: String,
        default_bool: bool,
        callback_id: u32,
        pre_init: bool,
    }

    unsafe extern "C++" {
        include!("bridge.h");

        // --- Opaque C++ types ---

        /// Opaque handle to a miral::Window on the C++ side.
        type MiralWindow;
        /// Opaque handle to a miral::WindowInfo on the C++ side.
        type MiralWindowInfo;
        /// Opaque handle to a miral::ApplicationInfo on the C++ side.
        type MiralApplicationInfo;
        /// Opaque handle to the policy tools on the C++ side.
        type MiralTools;
        /// Opaque handle to the Mir runner on the C++ side.
        type MiralRunner;

        // --- Runner lifecycle ---

        /// Create a new MirRunner from command-line arguments.
        fn miral_runner_new(args: &[String]) -> UniquePtr<MiralRunner>;
        /// Run the server with no policy. Blocks until exit.
        fn miral_runner_run(runner: Pin<&mut MiralRunner>) -> i32;
        /// Run the server with a Rust policy. Blocks until exit.
        fn miral_runner_run_with_rust_policy(runner: Pin<&mut MiralRunner>) -> i32;
        /// Run the server with a Rust policy and extensions configured.
        ///
        /// Extensions should be added via `miral_runner_add_*` before calling this.
        /// config_options: configuration option descriptors whose callbacks are in the global registry
        fn miral_runner_run_with_config(
            runner: Pin<&mut MiralRunner>,
            config_options: &[ConfigOptionDesc],
        ) -> i32;
        /// Tell the server to stop.
        fn miral_runner_stop(runner: Pin<&mut MiralRunner>);
        /// Register a start callback (will call rust_on_start_callback when server starts).
        fn miral_runner_register_start_callback(runner: Pin<&mut MiralRunner>);
        /// Register a stop callback (will call rust_on_stop_callback when server stops).
        fn miral_runner_register_stop_callback(runner: Pin<&mut MiralRunner>);

        /// Add decoration preferences to the runner.
        /// mode: 1=prefer_csd, 2=prefer_ssd, 3=always_ssd, 4=always_csd
        fn miral_runner_add_decorations(runner: Pin<&mut MiralRunner>, mode: i32);
        /// Add a keymap layout to the runner.
        fn miral_runner_add_keymap(runner: Pin<&mut MiralRunner>, layout: &str);
        /// Add X11/XWayland support to the runner.
        fn miral_runner_add_x11_support(runner: Pin<&mut MiralRunner>);
        /// Add Wayland extension defaults to the runner.
        fn miral_runner_add_wayland_extensions(
            runner: Pin<&mut MiralRunner>,
            enabled: &[String],
            disabled: &[String],
        );
        /// Add the external client launcher to the runner.
        fn miral_runner_add_external_launcher(runner: Pin<&mut MiralRunner>);
        /// Add the idle listener to the runner (callbacks set via global registry).
        fn miral_runner_add_idle_listener(runner: Pin<&mut MiralRunner>);
        /// Add the session lock listener to the runner (callbacks set via global registry).
        fn miral_runner_add_session_lock_listener(runner: Pin<&mut MiralRunner>);
        /// Add a magnifier to the runner with given settings.
        fn miral_runner_add_magnifier(
            runner: Pin<&mut MiralRunner>,
            magnification: f32,
            width: i32,
            height: i32,
            enabled: bool,
        );
        /// Add bounce keys accessibility support.
        fn miral_runner_add_bounce_keys(runner: Pin<&mut MiralRunner>, enabled: bool);
        /// Add slow keys accessibility support.
        fn miral_runner_add_slow_keys(runner: Pin<&mut MiralRunner>, enabled: bool);
        /// Add sticky keys accessibility support.
        fn miral_runner_add_sticky_keys(runner: Pin<&mut MiralRunner>, enabled: bool);
        /// Add mouse keys accessibility support.
        fn miral_runner_add_mousekeys(runner: Pin<&mut MiralRunner>, enabled: bool);
        /// Add locate pointer accessibility support.
        fn miral_runner_add_locate_pointer(runner: Pin<&mut MiralRunner>, enabled: bool);
        /// Add a cursor theme to the runner.
        fn miral_runner_add_cursor_theme(runner: Pin<&mut MiralRunner>, theme: &str);
        /// Add a cursor scale to the runner.
        fn miral_runner_add_cursor_scale(runner: Pin<&mut MiralRunner>, scale: f32);
        /// Add input configuration support to the runner.
        fn miral_runner_add_input_configuration(runner: Pin<&mut MiralRunner>);
        /// Add display configuration support to the runner.
        fn miral_runner_add_display_configuration(runner: Pin<&mut MiralRunner>);
        /// Add output filter support to the runner.
        fn miral_runner_add_output_filter(runner: Pin<&mut MiralRunner>);
        /// Add an init callback to the runner.
        fn miral_runner_add_init_callback(runner: Pin<&mut MiralRunner>);
        /// Add a terminator callback to the runner.
        fn miral_runner_add_terminator(runner: Pin<&mut MiralRunner>);

        /// Launch an external client by command string.
        ///
        /// Returns the pid of the launched process, or -1 on failure.
        /// Only valid after the server has started with the launcher enabled.
        fn miral_launcher_launch(command: &str) -> i32;

        /// Update the active magnifier enabled state.
        fn miral_magnifier_set_enabled(enabled: bool);
        /// Update the active magnifier zoom factor.
        fn miral_magnifier_set_magnification(magnification: f32);
        /// Update the active magnifier capture size.
        fn miral_magnifier_set_capture_size(width: i32, height: i32);

        // --- Window info queries ---

        /// Get a snapshot of window information.
        fn miral_window_info_snapshot(info: &MiralWindowInfo) -> WindowInfoSnapshot;
        /// Get the window handle from a WindowInfo.
        fn miral_window_info_window(info: &MiralWindowInfo) -> UniquePtr<MiralWindow>;
        /// Get the unique ID for the window in a WindowInfo.
        fn miral_window_info_id(info: &MiralWindowInfo) -> u64;
        /// Get the unique ID for an application.
        fn miral_app_info_id(info: &MiralApplicationInfo) -> u64;

        // --- Application info queries ---

        /// Get a snapshot of application information.
        fn miral_app_info_snapshot(info: &MiralApplicationInfo) -> ApplicationInfoSnapshot;

        // --- Window manager tools (ID-based for Rust interop) ---

        /// Get the number of connected applications.
        fn miral_tools_count_applications(tools: &MiralTools) -> u32;
        /// Get the active window info snapshot.
        fn miral_tools_active_window(tools: &MiralTools) -> WindowInfoSnapshot;
        /// Get the active window ID (0 if none).
        fn miral_tools_active_window_id(tools: &MiralTools) -> u64;
        /// Focus the next application.
        fn miral_tools_focus_next_application(tools: Pin<&mut MiralTools>);
        /// Focus the previous application.
        fn miral_tools_focus_prev_application(tools: Pin<&mut MiralTools>);
        /// Focus the next window within the current application.
        fn miral_tools_focus_next_within_application(tools: Pin<&mut MiralTools>);
        /// Focus the previous window within the current application.
        fn miral_tools_focus_prev_within_application(tools: Pin<&mut MiralTools>);
        /// Raise a window tree by ID.
        fn miral_tools_raise_tree_by_id(tools: Pin<&mut MiralTools>, window_id: u64);
        /// Ask a client to close its window by ID.
        fn miral_tools_ask_client_to_close_by_id(tools: Pin<&mut MiralTools>, window_id: u64);
        /// Modify a window with a specification by ID.
        fn miral_tools_modify_window_by_id(
            tools: Pin<&mut MiralTools>,
            window_id: u64,
            spec: &WindowSpecData,
        );
        /// Drag a window by a displacement, by ID.
        fn miral_tools_drag_window_by_id(
            tools: Pin<&mut MiralTools>,
            window_id: u64,
            movement: Displacement,
        );
        /// Select (focus) a window by ID.
        fn miral_tools_select_active_window_by_id(tools: Pin<&mut MiralTools>, window_id: u64);
        /// Find the window at a given point.
        fn miral_tools_window_at(tools: &MiralTools, point: Point) -> UniquePtr<MiralWindow>;
        /// Get the active output rectangle.
        fn miral_tools_active_output(tools: &MiralTools) -> Rectangle;
        /// Get the active application zone.
        fn miral_tools_active_application_zone(tools: &MiralTools) -> ZoneSnapshot;
        /// Get the info snapshot for a window by ID.
        fn miral_tools_info_for_window_id(tools: &MiralTools, window_id: u64)
            -> WindowInfoSnapshot;
        /// Swap two windows in the stacking order by their IDs.
        fn miral_tools_swap_tree_order_by_id(
            tools: Pin<&mut MiralTools>,
            window_id_a: u64,
            window_id_b: u64,
        );
        /// Send a window tree to the back by ID.
        fn miral_tools_send_tree_to_back_by_id(tools: Pin<&mut MiralTools>, window_id: u64);
        /// Start a drag operation on the active window.
        fn miral_tools_drag_active_window(tools: Pin<&mut MiralTools>, movement: Displacement);
        /// Move the cursor to a specific point.
        fn miral_tools_move_cursor_to(tools: Pin<&mut MiralTools>, point: Point);
        /// Get all window IDs (collected into a vector).
        fn miral_tools_all_window_ids(tools: &MiralTools) -> Vec<u64>;
        /// Get the window ID at a given point (0 if none).
        fn miral_tools_window_id_at(tools: &MiralTools, point: Point) -> u64;
        /// Compute placement and size for a window state transition.
        fn miral_tools_place_and_size_for_state(
            tools: &MiralTools,
            window_id: u64,
            new_state: i32,
            rect: &Rectangle,
        ) -> Rectangle;

        // --- Window queries ---

        /// Get the top-left position of a window.
        fn miral_window_top_left(window: &MiralWindow) -> Point;
        /// Get the size of a window.
        fn miral_window_size(window: &MiralWindow) -> Size;
        /// Check if a window is valid.
        fn miral_window_is_valid(window: &MiralWindow) -> bool;
        /// Get a unique ID for a window (for use as HashMap key).
        fn miral_window_id(window: &MiralWindow) -> u64;
    }

    // --- Rust exports to C++ (for policy dispatch) ---
    extern "Rust" {
        type RustPolicyHolder;

        /// Called by C++ to create the policy holder (reads from thread-local factory).
        fn rust_create_policy_holder() -> Box<RustPolicyHolder>;

        /// Called by C++ to set the tools pointer on the holder after creation.
        fn rust_policy_set_tools(holder: &mut RustPolicyHolder, tools_ptr: u64);

        fn rust_policy_place_new_window(
            holder: &mut RustPolicyHolder,
            app_info: &MiralApplicationInfo,
            spec: &WindowSpecData,
        ) -> WindowSpecData;

        fn rust_policy_handle_window_ready(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
        );

        fn rust_policy_handle_modify_window(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
            spec: &WindowSpecData,
        );

        fn rust_policy_handle_raise_window(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
        );

        fn rust_policy_handle_keyboard_event(
            holder: &mut RustPolicyHolder,
            event: &KeyboardEventInfo,
        ) -> bool;

        fn rust_policy_handle_touch_event(
            holder: &mut RustPolicyHolder,
            event: &TouchEventInfo,
        ) -> bool;

        fn rust_policy_handle_pointer_event(
            holder: &mut RustPolicyHolder,
            event: &PointerEventInfo,
        ) -> bool;

        fn rust_policy_handle_request_move(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
        );

        fn rust_policy_handle_request_resize(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
            edge: i32,
        );

        fn rust_policy_confirm_inherited_move(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
            movement: Displacement,
        ) -> Rectangle;

        fn rust_policy_confirm_placement_on_display(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
            new_state: i32,
            new_placement: &Rectangle,
        ) -> Rectangle;

        fn rust_policy_advise_new_app(
            holder: &mut RustPolicyHolder,
            app_info: &MiralApplicationInfo,
        );

        fn rust_policy_advise_delete_app(
            holder: &mut RustPolicyHolder,
            app_info: &MiralApplicationInfo,
        );

        fn rust_policy_advise_new_window(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
        );

        fn rust_policy_advise_delete_window(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
        );

        fn rust_policy_advise_focus_gained(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
        );

        fn rust_policy_advise_focus_lost(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
        );

        fn rust_policy_advise_state_change(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
            state: i32,
        );

        fn rust_policy_advise_move_to(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
            top_left: Point,
        );

        fn rust_policy_advise_resize(
            holder: &mut RustPolicyHolder,
            window_info: &MiralWindowInfo,
            new_size: Size,
        );

        fn rust_policy_advise_begin(holder: &mut RustPolicyHolder);
        fn rust_policy_advise_end(holder: &mut RustPolicyHolder);

        fn rust_policy_advise_output_create(holder: &mut RustPolicyHolder, output: &OutputSnapshot);
        fn rust_policy_advise_output_update(
            holder: &mut RustPolicyHolder,
            updated: &OutputSnapshot,
            original: &OutputSnapshot,
        );
        fn rust_policy_advise_output_delete(holder: &mut RustPolicyHolder, output: &OutputSnapshot);

        fn rust_policy_advise_zone_create(holder: &mut RustPolicyHolder, zone: &ZoneSnapshot);
        fn rust_policy_advise_zone_update(
            holder: &mut RustPolicyHolder,
            updated: &ZoneSnapshot,
            original: &ZoneSnapshot,
        );
        fn rust_policy_advise_zone_delete(holder: &mut RustPolicyHolder, zone: &ZoneSnapshot);

        fn rust_policy_advise_adding_to_workspace(
            holder: &mut RustPolicyHolder,
            window_ids: &[u64],
        );
        fn rust_policy_advise_removing_from_workspace(
            holder: &mut RustPolicyHolder,
            window_ids: &[u64],
        );

        /// Called from C++ when the server has started.
        fn rust_on_start_callback();
        /// Called from C++ when the server is stopping.
        fn rust_on_stop_callback();
        /// Called from C++ when the display is about to dim.
        fn rust_on_idle_dim_callback();
        /// Called from C++ when the display is turned off.
        fn rust_on_idle_off_callback();
        /// Called from C++ when the display wakes up.
        fn rust_on_idle_wake_callback();
        /// Called from C++ when the session is locked.
        fn rust_on_session_lock_callback();
        /// Called from C++ when the session is unlocked.
        fn rust_on_session_unlock_callback();
        /// Called from C++ when the init callback should run.
        fn rust_on_init_callback();
        /// Called from C++ when the terminator callback should run.
        fn rust_on_terminator_callback(signal: i32);

        // --- Configuration option callbacks (called from C++) ---

        /// Dispatch an int config option callback.
        fn rust_config_callback_int(callback_id: u32, value: i32);
        /// Dispatch a double config option callback.
        fn rust_config_callback_double(callback_id: u32, value: f64);
        /// Dispatch a string config option callback.
        fn rust_config_callback_string(callback_id: u32, value: &str);
        /// Dispatch a bool config option callback.
        fn rust_config_callback_bool(callback_id: u32, value: bool);
        /// Dispatch a flag config option callback.
        fn rust_config_callback_flag(callback_id: u32, is_set: bool);
        /// Dispatch an optional int config option callback.
        fn rust_config_callback_optional_int(callback_id: u32, has_value: bool, value: i32);
        /// Dispatch an optional string config option callback.
        fn rust_config_callback_optional_string(callback_id: u32, has_value: bool, value: &str);
        /// Dispatch an optional bool config option callback.
        fn rust_config_callback_optional_bool(callback_id: u32, has_value: bool, value: bool);
        /// Dispatch a multi-value string list config option callback.
        fn rust_config_callback_multi(callback_id: u32, values: &[String]);
    }
}

/// The Rust-side policy holder that wraps a trait object.
///
/// This is the type that C++ holds a reference to and dispatches virtual calls through.
pub struct RustPolicyHolder {
    /// The actual policy implementation, provided by the compositor author.
    pub policy: Box<dyn PolicyBridge>,
    /// Raw pointer to the C++ MiralTools object (set by C++ after construction).
    /// Valid for the lifetime of the runner. Only used by the `miral` crate.
    pub tools_ptr: u64,
}

/// Internal trait used by the FFI layer to dispatch policy calls.
///
/// This mirrors the public `WindowManagementPolicy` trait from the `miral` crate
/// but uses FFI-compatible types.
pub trait PolicyBridge: Send {
    /// Called after tools_ptr has been set on the holder — gives the policy a chance
    /// to store a reference to the tools.
    fn set_tools_ptr(&mut self, tools_ptr: u64);

    fn place_new_window(
        &mut self,
        app_info: &ffi::MiralApplicationInfo,
        spec: &ffi::WindowSpecData,
    ) -> ffi::WindowSpecData;
    fn handle_window_ready(&mut self, window_info: &ffi::MiralWindowInfo);
    fn handle_modify_window(
        &mut self,
        window_info: &ffi::MiralWindowInfo,
        spec: &ffi::WindowSpecData,
    );
    fn handle_raise_window(&mut self, window_info: &ffi::MiralWindowInfo);
    fn handle_keyboard_event(&mut self, event: &ffi::KeyboardEventInfo) -> bool;
    fn handle_touch_event(&mut self, event: &ffi::TouchEventInfo) -> bool;
    fn handle_pointer_event(&mut self, event: &ffi::PointerEventInfo) -> bool;
    fn handle_request_move(&mut self, window_info: &ffi::MiralWindowInfo);
    fn handle_request_resize(&mut self, window_info: &ffi::MiralWindowInfo, edge: i32);
    fn confirm_inherited_move(
        &mut self,
        window_info: &ffi::MiralWindowInfo,
        movement: ffi::Displacement,
    ) -> ffi::Rectangle;
    fn confirm_placement_on_display(
        &mut self,
        window_info: &ffi::MiralWindowInfo,
        new_state: i32,
        new_placement: &ffi::Rectangle,
    ) -> ffi::Rectangle;
    fn advise_new_app(&mut self, app_info: &ffi::MiralApplicationInfo);
    fn advise_delete_app(&mut self, app_info: &ffi::MiralApplicationInfo);
    fn advise_new_window(&mut self, window_info: &ffi::MiralWindowInfo);
    fn advise_delete_window(&mut self, window_info: &ffi::MiralWindowInfo);
    fn advise_focus_gained(&mut self, window_info: &ffi::MiralWindowInfo);
    fn advise_focus_lost(&mut self, window_info: &ffi::MiralWindowInfo);
    fn advise_state_change(&mut self, window_info: &ffi::MiralWindowInfo, state: i32);
    fn advise_move_to(&mut self, window_info: &ffi::MiralWindowInfo, top_left: ffi::Point);
    fn advise_resize(&mut self, window_info: &ffi::MiralWindowInfo, new_size: ffi::Size);
    fn advise_begin(&mut self);
    fn advise_end(&mut self);
    fn advise_output_create(&mut self, output: &ffi::OutputSnapshot);
    fn advise_output_update(
        &mut self,
        updated: &ffi::OutputSnapshot,
        original: &ffi::OutputSnapshot,
    );
    fn advise_output_delete(&mut self, output: &ffi::OutputSnapshot);
    fn advise_zone_create(&mut self, zone: &ffi::ZoneSnapshot);
    fn advise_zone_update(&mut self, updated: &ffi::ZoneSnapshot, original: &ffi::ZoneSnapshot);
    fn advise_zone_delete(&mut self, zone: &ffi::ZoneSnapshot);
    fn advise_adding_to_workspace(&mut self, window_ids: &[u64]);
    fn advise_removing_from_workspace(&mut self, window_ids: &[u64]);
}

// --- Thread-local policy factory ---

use std::cell::RefCell;

type PolicyFactoryFn = Box<dyn FnOnce() -> Box<dyn PolicyBridge>>;
type LifecycleCallback = Box<dyn FnOnce() + Send>;
type RepeatingCallback = Box<dyn Fn() + Send>;
type SignalCallback = Box<dyn Fn(i32) + Send>;

thread_local! {
    static POLICY_FACTORY: RefCell<Option<PolicyFactoryFn>> = RefCell::new(None);
    static ON_START_CALLBACK: RefCell<Option<LifecycleCallback>> = RefCell::new(None);
    static ON_STOP_CALLBACK: RefCell<Option<LifecycleCallback>> = RefCell::new(None);
    static ON_IDLE_DIM_CALLBACK: RefCell<Option<RepeatingCallback>> = RefCell::new(None);
    static ON_IDLE_OFF_CALLBACK: RefCell<Option<RepeatingCallback>> = RefCell::new(None);
    static ON_IDLE_WAKE_CALLBACK: RefCell<Option<RepeatingCallback>> = RefCell::new(None);
    static ON_SESSION_LOCK_CALLBACK: RefCell<Option<RepeatingCallback>> = RefCell::new(None);
    static ON_SESSION_UNLOCK_CALLBACK: RefCell<Option<RepeatingCallback>> = RefCell::new(None);
    static ON_INIT_CALLBACKS: RefCell<Vec<LifecycleCallback>> = RefCell::new(Vec::new());
    static ON_TERMINATOR_CALLBACK: RefCell<Option<SignalCallback>> = RefCell::new(None);
}

/// Set the policy factory that will be called when C++ creates the policy.
/// Must be called before `miral_runner_run_with_rust_policy`.
pub fn set_policy_factory(factory: PolicyFactoryFn) {
    POLICY_FACTORY.with(|f| {
        *f.borrow_mut() = Some(factory);
    });
}

/// Set the start callback, to be invoked when the server starts.
pub fn set_on_start_callback(callback: LifecycleCallback) {
    ON_START_CALLBACK.with(|f| {
        *f.borrow_mut() = Some(callback);
    });
}

/// Set the stop callback, to be invoked when the server stops.
pub fn set_on_stop_callback(callback: LifecycleCallback) {
    ON_STOP_CALLBACK.with(|f| {
        *f.borrow_mut() = Some(callback);
    });
}

/// Set the callback for when the display is about to dim.
pub fn set_on_idle_dim_callback(callback: RepeatingCallback) {
    ON_IDLE_DIM_CALLBACK.with(|f| {
        *f.borrow_mut() = Some(callback);
    });
}

/// Set the callback for when the display is turned off.
pub fn set_on_idle_off_callback(callback: RepeatingCallback) {
    ON_IDLE_OFF_CALLBACK.with(|f| {
        *f.borrow_mut() = Some(callback);
    });
}

/// Set the callback for when the display wakes up.
pub fn set_on_idle_wake_callback(callback: RepeatingCallback) {
    ON_IDLE_WAKE_CALLBACK.with(|f| {
        *f.borrow_mut() = Some(callback);
    });
}

/// Set the callback for when the session is locked.
pub fn set_on_session_lock_callback(callback: RepeatingCallback) {
    ON_SESSION_LOCK_CALLBACK.with(|f| {
        *f.borrow_mut() = Some(callback);
    });
}

/// Set the callback for when the session is unlocked.
pub fn set_on_session_unlock_callback(callback: RepeatingCallback) {
    ON_SESSION_UNLOCK_CALLBACK.with(|f| {
        *f.borrow_mut() = Some(callback);
    });
}

/// Set the callback for when the server init hook runs.
pub fn set_on_init_callback(callback: LifecycleCallback) {
    ON_INIT_CALLBACKS.with(|f| {
        f.borrow_mut().push(callback);
    });
}

/// Set the callback for termination requests.
pub fn set_on_terminator_callback(callback: SignalCallback) {
    ON_TERMINATOR_CALLBACK.with(|f| {
        *f.borrow_mut() = Some(callback);
    });
}

/// Clear all runner-scoped callbacks.
pub fn clear_runner_callbacks() {
    ON_START_CALLBACK.with(|f| *f.borrow_mut() = None);
    ON_STOP_CALLBACK.with(|f| *f.borrow_mut() = None);
    ON_IDLE_DIM_CALLBACK.with(|f| *f.borrow_mut() = None);
    ON_IDLE_OFF_CALLBACK.with(|f| *f.borrow_mut() = None);
    ON_IDLE_WAKE_CALLBACK.with(|f| *f.borrow_mut() = None);
    ON_SESSION_LOCK_CALLBACK.with(|f| *f.borrow_mut() = None);
    ON_SESSION_UNLOCK_CALLBACK.with(|f| *f.borrow_mut() = None);
}

// --- Functions called from C++ ---

fn rust_create_policy_holder() -> Box<RustPolicyHolder> {
    let policy = POLICY_FACTORY.with(|f| {
        let factory = f
            .borrow_mut()
            .take()
            .expect("No policy factory registered. Call set_policy_factory before run.");
        factory()
    });
    Box::new(RustPolicyHolder {
        policy,
        tools_ptr: 0,
    })
}

fn rust_policy_set_tools(holder: &mut RustPolicyHolder, tools_ptr: u64) {
    holder.tools_ptr = tools_ptr;
    holder.policy.set_tools_ptr(tools_ptr);
}

// --- Policy dispatch functions called from C++ ---

fn rust_policy_place_new_window(
    holder: &mut RustPolicyHolder,
    app_info: &ffi::MiralApplicationInfo,
    spec: &ffi::WindowSpecData,
) -> ffi::WindowSpecData {
    holder.policy.place_new_window(app_info, spec)
}

fn rust_policy_handle_window_ready(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
) {
    holder.policy.handle_window_ready(window_info);
}

fn rust_policy_handle_modify_window(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
    spec: &ffi::WindowSpecData,
) {
    holder.policy.handle_modify_window(window_info, spec);
}

fn rust_policy_handle_raise_window(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
) {
    holder.policy.handle_raise_window(window_info);
}

fn rust_policy_handle_keyboard_event(
    holder: &mut RustPolicyHolder,
    event: &ffi::KeyboardEventInfo,
) -> bool {
    holder.policy.handle_keyboard_event(event)
}

fn rust_policy_handle_touch_event(
    holder: &mut RustPolicyHolder,
    event: &ffi::TouchEventInfo,
) -> bool {
    holder.policy.handle_touch_event(event)
}

fn rust_policy_handle_pointer_event(
    holder: &mut RustPolicyHolder,
    event: &ffi::PointerEventInfo,
) -> bool {
    holder.policy.handle_pointer_event(event)
}

fn rust_policy_handle_request_move(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
) {
    holder.policy.handle_request_move(window_info);
}

fn rust_policy_handle_request_resize(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
    edge: i32,
) {
    holder.policy.handle_request_resize(window_info, edge);
}

fn rust_policy_confirm_inherited_move(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
    movement: ffi::Displacement,
) -> ffi::Rectangle {
    holder.policy.confirm_inherited_move(window_info, movement)
}

fn rust_policy_confirm_placement_on_display(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
    new_state: i32,
    new_placement: &ffi::Rectangle,
) -> ffi::Rectangle {
    holder
        .policy
        .confirm_placement_on_display(window_info, new_state, new_placement)
}

fn rust_policy_advise_new_app(holder: &mut RustPolicyHolder, app_info: &ffi::MiralApplicationInfo) {
    holder.policy.advise_new_app(app_info);
}

fn rust_policy_advise_delete_app(
    holder: &mut RustPolicyHolder,
    app_info: &ffi::MiralApplicationInfo,
) {
    holder.policy.advise_delete_app(app_info);
}

fn rust_policy_advise_new_window(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
) {
    holder.policy.advise_new_window(window_info);
}

fn rust_policy_advise_delete_window(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
) {
    holder.policy.advise_delete_window(window_info);
}

fn rust_policy_advise_focus_gained(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
) {
    holder.policy.advise_focus_gained(window_info);
}

fn rust_policy_advise_focus_lost(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
) {
    holder.policy.advise_focus_lost(window_info);
}

fn rust_policy_advise_state_change(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
    state: i32,
) {
    holder.policy.advise_state_change(window_info, state);
}

fn rust_policy_advise_move_to(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
    top_left: ffi::Point,
) {
    holder.policy.advise_move_to(window_info, top_left);
}

fn rust_policy_advise_resize(
    holder: &mut RustPolicyHolder,
    window_info: &ffi::MiralWindowInfo,
    new_size: ffi::Size,
) {
    holder.policy.advise_resize(window_info, new_size);
}

fn rust_policy_advise_begin(holder: &mut RustPolicyHolder) {
    holder.policy.advise_begin();
}

fn rust_policy_advise_end(holder: &mut RustPolicyHolder) {
    holder.policy.advise_end();
}

fn rust_policy_advise_output_create(holder: &mut RustPolicyHolder, output: &ffi::OutputSnapshot) {
    holder.policy.advise_output_create(output);
}

fn rust_policy_advise_output_update(
    holder: &mut RustPolicyHolder,
    updated: &ffi::OutputSnapshot,
    original: &ffi::OutputSnapshot,
) {
    holder.policy.advise_output_update(updated, original);
}

fn rust_policy_advise_output_delete(holder: &mut RustPolicyHolder, output: &ffi::OutputSnapshot) {
    holder.policy.advise_output_delete(output);
}

fn rust_policy_advise_zone_create(holder: &mut RustPolicyHolder, zone: &ffi::ZoneSnapshot) {
    holder.policy.advise_zone_create(zone);
}

fn rust_policy_advise_zone_update(
    holder: &mut RustPolicyHolder,
    updated: &ffi::ZoneSnapshot,
    original: &ffi::ZoneSnapshot,
) {
    holder.policy.advise_zone_update(updated, original);
}

fn rust_policy_advise_zone_delete(holder: &mut RustPolicyHolder, zone: &ffi::ZoneSnapshot) {
    holder.policy.advise_zone_delete(zone);
}

fn rust_policy_advise_adding_to_workspace(holder: &mut RustPolicyHolder, window_ids: &[u64]) {
    holder.policy.advise_adding_to_workspace(window_ids);
}

fn rust_policy_advise_removing_from_workspace(holder: &mut RustPolicyHolder, window_ids: &[u64]) {
    holder.policy.advise_removing_from_workspace(window_ids);
}

// --- Lifecycle callbacks called from C++ ---

fn rust_on_start_callback() {
    ON_START_CALLBACK.with(|f| {
        if let Some(callback) = f.borrow_mut().take() {
            callback();
        }
    });
}

fn rust_on_stop_callback() {
    ON_STOP_CALLBACK.with(|f| {
        if let Some(callback) = f.borrow_mut().take() {
            callback();
        }
    });
}

fn rust_on_idle_dim_callback() {
    ON_IDLE_DIM_CALLBACK.with(|f| {
        if let Some(callback) = f.borrow().as_ref() {
            callback();
        }
    });
}

fn rust_on_idle_off_callback() {
    ON_IDLE_OFF_CALLBACK.with(|f| {
        if let Some(callback) = f.borrow().as_ref() {
            callback();
        }
    });
}

fn rust_on_idle_wake_callback() {
    ON_IDLE_WAKE_CALLBACK.with(|f| {
        if let Some(callback) = f.borrow().as_ref() {
            callback();
        }
    });
}

fn rust_on_session_lock_callback() {
    ON_SESSION_LOCK_CALLBACK.with(|f| {
        if let Some(callback) = f.borrow().as_ref() {
            callback();
        }
    });
}

fn rust_on_session_unlock_callback() {
    ON_SESSION_UNLOCK_CALLBACK.with(|f| {
        if let Some(callback) = f.borrow().as_ref() {
            callback();
        }
    });
}

fn rust_on_init_callback() {
    ON_INIT_CALLBACKS.with(|f| {
        let mut callbacks = f.borrow_mut();
        if !callbacks.is_empty() {
            let callback = callbacks.remove(0);
            callback();
        }
    });
}

fn rust_on_terminator_callback(signal: i32) {
    ON_TERMINATOR_CALLBACK.with(|f| {
        if let Some(callback) = f.borrow().as_ref() {
            callback(signal);
        }
    });
}

// --- Configuration option callback registry ---

use std::sync::Mutex;

/// Typed callback variants for configuration options.
pub enum ConfigCallback {
    /// Callback for an `i32` configuration option.
    Int(Box<dyn FnMut(i32) + Send>),
    /// Callback for an `f64` configuration option.
    Double(Box<dyn FnMut(f64) + Send>),
    /// Callback for a `String` configuration option.
    Str(Box<dyn FnMut(String) + Send>),
    /// Callback for a `bool` configuration option.
    Bool(Box<dyn FnMut(bool) + Send>),
    /// Callback for a presence-only flag option.
    Flag(Box<dyn FnMut(bool) + Send>),
    /// Callback for an optional `i32` configuration option.
    OptionalInt(Box<dyn FnMut(Option<i32>) + Send>),
    /// Callback for an optional `String` configuration option.
    OptionalStr(Box<dyn FnMut(Option<String>) + Send>),
    /// Callback for an optional `bool` configuration option.
    OptionalBool(Box<dyn FnMut(Option<bool>) + Send>),
    /// Callback for a multi-value string list configuration option.
    Multi(Box<dyn FnMut(Vec<String>) + Send>),
}

static CONFIG_CALLBACKS: Mutex<Vec<ConfigCallback>> = Mutex::new(Vec::new());

/// Register a config callback and return its ID.
///
/// Called by the `miral` crate when building `ConfigurationOption` values.
/// The returned ID is stored in a `ConfigOptionDesc` and used by C++ to
/// dispatch back into the correct Rust callback.
pub fn register_config_callback(callback: ConfigCallback) -> u32 {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    let id = callbacks.len() as u32;
    callbacks.push(callback);
    id
}

/// Clear all registered config callbacks.
///
/// Called by the `miral` crate after `run()` completes to avoid leaking callbacks.
pub fn clear_config_callbacks() {
    CONFIG_CALLBACKS.lock().unwrap().clear();
}

// --- Config callback dispatch functions called from C++ ---

fn rust_config_callback_int(callback_id: u32, value: i32) {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    if let Some(ConfigCallback::Int(cb)) = callbacks.get_mut(callback_id as usize) {
        cb(value);
    }
}

fn rust_config_callback_double(callback_id: u32, value: f64) {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    if let Some(ConfigCallback::Double(cb)) = callbacks.get_mut(callback_id as usize) {
        cb(value);
    }
}

fn rust_config_callback_string(callback_id: u32, value: &str) {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    if let Some(ConfigCallback::Str(cb)) = callbacks.get_mut(callback_id as usize) {
        cb(value.to_string());
    }
}

fn rust_config_callback_bool(callback_id: u32, value: bool) {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    if let Some(ConfigCallback::Bool(cb)) = callbacks.get_mut(callback_id as usize) {
        cb(value);
    }
}

fn rust_config_callback_flag(callback_id: u32, is_set: bool) {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    if let Some(ConfigCallback::Flag(cb)) = callbacks.get_mut(callback_id as usize) {
        cb(is_set);
    }
}

fn rust_config_callback_optional_int(callback_id: u32, has_value: bool, value: i32) {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    if let Some(ConfigCallback::OptionalInt(cb)) = callbacks.get_mut(callback_id as usize) {
        cb(if has_value { Some(value) } else { None });
    }
}

fn rust_config_callback_optional_string(callback_id: u32, has_value: bool, value: &str) {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    if let Some(ConfigCallback::OptionalStr(cb)) = callbacks.get_mut(callback_id as usize) {
        cb(if has_value {
            Some(value.to_string())
        } else {
            None
        });
    }
}

fn rust_config_callback_optional_bool(callback_id: u32, has_value: bool, value: bool) {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    if let Some(ConfigCallback::OptionalBool(cb)) = callbacks.get_mut(callback_id as usize) {
        cb(if has_value { Some(value) } else { None });
    }
}

fn rust_config_callback_multi(callback_id: u32, values: &[String]) {
    let mut callbacks = CONFIG_CALLBACKS.lock().unwrap();
    if let Some(ConfigCallback::Multi(cb)) = callbacks.get_mut(callback_id as usize) {
        cb(values.to_vec());
    }
}
