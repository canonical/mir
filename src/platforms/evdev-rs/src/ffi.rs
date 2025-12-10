/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

use cxx::ExternType;

#[repr(C)]
#[derive(Copy, Clone)]
pub struct KeyEventData {
    pub has_time: bool,
    pub time_microseconds: u64,
    pub action: i32,
    pub scancode: u32,
}

unsafe impl ExternType for KeyEventData {
    type Id = cxx::type_id!("mir::input::evdev_rs::KeyEventData");
    type Kind = cxx::kind::Trivial;
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct PointerEventDataRs {
    pub has_time: bool,
    pub time_microseconds: u64,
    pub action: i32,
    pub buttons: u32,
    pub has_position: bool,
    pub position_x: f32,
    pub position_y: f32,
    pub displacement_x: f32,
    pub displacement_y: f32,
    pub axis_source: i32,
    pub precise_x: f32,
    pub discrete_x: i32,
    pub value120_x: i32,
    pub scroll_stop_x: bool,
    pub precise_y: f32,
    pub discrete_y: i32,
    pub value120_y: i32,
    pub scroll_stop_y: bool,
}

unsafe impl ExternType for PointerEventDataRs {
    type Id = cxx::type_id!("mir::input::evdev_rs::PointerEventData");
    type Kind = cxx::kind::Trivial;
}

#[cxx::bridge(namespace = "mir::input::evdev_rs")]
pub mod bridge {
    #[repr(u32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum DeviceCapability {
        unknown = 0,
        pointer = 2,
        keyboard = 4,
        touchpad = 8,
        touchscreen = 16,
        gamepad = 32,
        joystick = 64,
        switch_ = 128,
        multitouch = 256,
        alpha_numeric = 512,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum MirKeyboardAction {
        mir_keyboard_action_up,
        mir_keyboard_action_down,
        mir_keyboard_action_repeat,
        mir_keyboard_action_modifiers,
        mir_keyboard_actions,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum MirPointerAcceleration {
        mir_pointer_acceleration_none = 1,
        mir_pointer_acceleration_adaptive = 2,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum MirPointerAction {
        mir_pointer_action_button_up,
        mir_pointer_action_button_down,
        mir_pointer_action_enter,
        mir_pointer_action_leave,
        mir_pointer_action_motion,
        mir_pointer_actions,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum MirPointerAxisSource {
        mir_pointer_axis_source_none,
        mir_pointer_axis_source_wheel,
        mir_pointer_axis_source_finger,
        mir_pointer_axis_source_continuous,
        mir_pointer_axis_source_wheel_tilt,
    }

    #[repr(u32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum MirPointerButton {
        mir_pointer_button_primary = 1,
        mir_pointer_button_secondary = 2,
        mir_pointer_button_tertiary = 4,
        mir_pointer_button_back = 8,
        mir_pointer_button_forward = 16,
        mir_pointer_button_side = 32,
        mir_pointer_button_extra = 64,
        mir_pointer_button_task = 128,
    }

    #[repr(i32)]
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum MirPointerHandedness {
        mir_pointer_handedness_right = 0,
        mir_pointer_handedness_left = 1,
    }

    #[derive(Copy, Clone)]
    pub struct PointerSettings {
        pub handedness: i32,
        pub cursor_acceleration_bias: f64,
        pub acceleration: i32,
        pub horizontal_scroll_scale: f64,
        pub vertical_scroll_scale: f64,
    }

    #[derive(Copy, Clone)]
    pub struct PointerSettingsRs {
        pub is_set: bool,
        pub handedness: i32,
        pub cursor_acceleration_bias: f64,
        pub acceleration: i32,
        pub horizontal_scroll_scale: f64,
        pub vertical_scroll_scale: f64,
        pub has_error: bool,
    }

    // PointerEventData is declared as an extern C++ type mapped to the Rust struct above

    // KeyEventData is declared as an extern C++ type mapped to the Rust struct above

    extern "Rust" {
        type PlatformRs;
        type DeviceObserverRs;
        type InputDeviceRs;
        type InputDeviceInfoRs;

        fn start(self: &mut PlatformRs);
        fn continue_after_config(self: &PlatformRs);
        fn pause_for_config(self: &PlatformRs);
        fn stop(self: &mut PlatformRs);
        unsafe fn libinput_fd(self: &mut PlatformRs) -> i32;
        pub fn process(self: &mut PlatformRs);
        fn create_device_observer(self: &PlatformRs) -> Box<DeviceObserverRs>;
        fn create_input_device(self: &mut PlatformRs, device_id: i32) -> Box<InputDeviceRs>;

        fn activated(self: &mut DeviceObserverRs, fd: i32);
        fn suspended(self: &mut DeviceObserverRs);
        fn removed(self: &mut DeviceObserverRs);

        /// # Safety
        ///
        /// This is unsafe because it is receiving raw pointers from C++ as parameters.
        unsafe fn start(
            self: &mut InputDeviceRs,
            input_sink: *mut InputSink,
            event_builder: *mut EventBuilder,
        );
        fn stop(self: &mut InputDeviceRs);
        fn get_device_info(self: &InputDeviceRs) -> Box<InputDeviceInfoRs>;

        fn valid(self: &InputDeviceInfoRs) -> bool;
        fn name(self: &InputDeviceInfoRs) -> &str;
        fn unique_id(self: &InputDeviceInfoRs) -> &str;
        fn capabilities(self: &InputDeviceInfoRs) -> u32;
        fn get_pointer_settings(self: &InputDeviceRs) -> Box<PointerSettingsRs>;
        fn set_pointer_settings(self: &InputDeviceRs, settings: &PointerSettings);

        fn evdev_rs_create(
            bridge: SharedPtr<PlatformBridge>,
            device_registry: SharedPtr<InputDeviceRegistry>,
        ) -> Box<PlatformRs>;
    }

    unsafe extern "C++" {
        include!("platform_bridge.h");
        include!("mir/input/input_device_registry.h");
        include!("mir/input/device_capability.h");
        include!("mir/input/input_sink.h");
        include!("mir/input/event_builder.h");
        include!("mir/events/event.h");
        include!("mir/events/scroll_axis.h");
        include!("mir_toolkit/events/enums.h");

        pub type PlatformBridge;
        pub type DeviceWrapper;
        pub type EventBuilderWrapper;
        pub type RectangleWrapper;
        // Map C++ KeyEventData to the Rust struct
        type KeyEventData = crate::ffi::KeyEventData;
        // Map C++ PointerEventData to the Rust struct
        type PointerEventData = crate::ffi::PointerEventDataRs;

        #[namespace = "mir::input"]
        pub type Device;

        #[namespace = "mir::input"]
        pub type InputDevice;

        #[namespace = "mir::input"]
        pub type InputDeviceRegistry;

        #[namespace = "mir::input"]
        pub type InputSink;

        #[namespace = "mir::input"]
        pub type EventBuilder;

        #[namespace = ""]
        pub type MirEvent;

        pub fn acquire_device(
            self: &PlatformBridge,
            major: i32,
            minor: i32,
        ) -> UniquePtr<DeviceWrapper>;
        pub fn bounding_rectangle(
            self: &PlatformBridge,
            sink: &InputSink,
        ) -> UniquePtr<RectangleWrapper>;
        pub fn create_input_device(self: &PlatformBridge, device_id: i32) -> SharedPtr<InputDevice>;
        pub fn raw_fd(self: &DeviceWrapper) -> i32;

        // # Safety
        //
        // This is unsafe because it receives a raw C++ pointer as an argument.
        pub unsafe fn create_event_builder_wrapper(
            self: &PlatformBridge,
            event_builder: *mut EventBuilder,
        ) -> UniquePtr<EventBuilderWrapper>;

        pub fn pointer_event(
            self: &EventBuilderWrapper,
            data: &PointerEventData,
        ) -> SharedPtr<MirEvent>;

        pub fn key_event(self: &EventBuilderWrapper, data: &KeyEventData) -> SharedPtr<MirEvent>;

        #[namespace = "mir::input"]
        pub fn add_device(
            self: Pin<&mut InputDeviceRegistry>,
            device: &SharedPtr<InputDevice>,
        ) -> WeakPtr<Device>;

        #[namespace = "mir::input"]
        pub fn remove_device(self: Pin<&mut InputDeviceRegistry>, device: &SharedPtr<InputDevice>);

        #[namespace = "mir::input"]
        pub fn handle_input(self: Pin<&mut InputSink>, event: &SharedPtr<MirEvent>);

        pub fn x(self: &RectangleWrapper) -> i32;
        pub fn y(self: &RectangleWrapper) -> i32;
        pub fn width(self: &RectangleWrapper) -> i32;
        pub fn height(self: &RectangleWrapper) -> i32;
    }
}

impl bridge::PointerSettingsRs {
    pub fn empty() -> Self {
        bridge::PointerSettingsRs {
            is_set: false,
            handedness: 0,
            cursor_acceleration_bias: 0.0,
            acceleration: bridge::MirPointerAcceleration::mir_pointer_acceleration_none.repr,
            horizontal_scroll_scale: 1.0,
            vertical_scroll_scale: 1.0,
            has_error: false,
        }
    }
}
