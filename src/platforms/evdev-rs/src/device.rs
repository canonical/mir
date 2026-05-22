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

use cxx::SharedPtr;
use std::collections::HashMap;
use std::sync::Arc;
use std::sync::Mutex;

use crate::{
    DeviceCapability, EventBuilder, EventBuilderWrapper, InputDevice, InputSink, MirEvent,
    MirPointerAcceleration, MirPointerHandedness, MirTouchAction, MirTouchTooltype, PlatformBridge,
    PointerSettings, SetPointerSettingsData,
};

/// The libinput device.
pub struct LibinputDevice {
    // The id of the device.
    pub device_id: i32,

    /// The current state of the device, which must be thread safe.
    pub state: Arc<Mutex<LibinputDeviceState>>,

    /// A reference to the bridge that communicates with C++.
    pub bridge: SharedPtr<PlatformBridge>,
}

impl LibinputDevice {
    pub fn start(&mut self, input_sink: *mut InputSink, event_builder: *mut EventBuilder) {
        let Ok(mut guard) = self.state.lock() else {
            eprintln!("LibinputDevice::start: unable to acquire state lock; lock poisoned");
            return;
        };

        let Some(device_info) = guard.find_device_by_id(self.device_id) else {
            eprintln!(
                "LibinputDevice::start: device id {} not found",
                self.device_id
            );
            return;
        };

        if let Some(nn) = std::ptr::NonNull::new(input_sink) {
            device_info.input_sink = Some(InputSinkPtr(nn));
        } else {
            eprintln!("LibinputDevice::start: input_sink pointer is null; not starting sink");
        }

        if event_builder.is_null() {
            eprintln!(
                "LibinputDevice::start: event_builder pointer is null; not creating event builder"
            );
        } else {
            // # Safety
            //
            // Calling create_event_builder_wrapper with a raw pointer is unsafe.
            device_info.event_builder = Some(EventBuilderWrapperPtr(unsafe {
                self.bridge.create_event_builder_wrapper(event_builder)
            }));
        }
    }

    pub fn stop(&mut self) {}

    pub fn get_device_info(&self) -> Box<LibinputDeviceMetadata> {
        let Ok(mut guard) = self.state.lock() else {
            eprintln!(
                "LibinputDevice::get_device_info: unable to acquire state lock; lock poisoned"
            );
            return Box::new(LibinputDeviceMetadata::default());
        };

        let Some(device_info) = guard.find_device_by_id(self.device_id) else {
            eprintln!(
                "LibinputDevice::get_device_info: device id {} not found",
                self.device_id
            );
            return Box::new(LibinputDeviceMetadata::default());
        };

        let mut capabilities: u32 = 0;
        if device_info
            .device
            .has_capability(input::DeviceCapability::Keyboard)
        {
            capabilities |= DeviceCapability::keyboard.repr | DeviceCapability::alpha_numeric.repr;
        }
        if device_info
            .device
            .has_capability(input::DeviceCapability::Pointer)
        {
            capabilities |= DeviceCapability::pointer.repr;
        }
        if device_info
            .device
            .has_capability(input::DeviceCapability::Touch)
        {
            capabilities |= DeviceCapability::touchpad.repr | DeviceCapability::pointer.repr;
        }

        Box::new(LibinputDeviceMetadata {
            name: device_info.device.name().to_string(),
            unique_id: format!(
                "{} {} {} {}",
                device_info.device.name(),
                device_info.device.sysname(),
                device_info.device.id_vendor(),
                device_info.device.id_product()
            ),
            capabilities,
            valid: true,
        })
    }

    pub fn get_pointer_settings(&self) -> Box<PointerSettings> {
        let Ok(mut guard) = self.state.lock() else {
            eprintln!(
                "LibinputDevice::get_pointer_settings: unable to acquire state lock; lock poisoned"
            );
            return Box::new(PointerSettings {
                has_error: true,
                ..Default::default()
            });
        };

        let Some(device_info) = guard.find_device_by_id(self.device_id) else {
            eprintln!(
                "Attempting to get pointer settings from a device that is not pointer capable."
            );
            return Box::new(PointerSettings::default());
        };

        if !device_info
            .device
            .has_capability(input::DeviceCapability::Pointer)
        {
            eprintln!(
                "Attempting to get pointer settings from a device that is not pointer capable."
            );
            return Box::new(PointerSettings::default());
        }

        let handedness = if device_info.device.config_left_handed() {
            MirPointerHandedness::mir_pointer_handedness_left.repr
        } else {
            MirPointerHandedness::mir_pointer_handedness_right.repr
        };

        let acceleration = match device_info.device.config_accel_profile() {
            Some(input::AccelProfile::Adaptive) => {
                MirPointerAcceleration::mir_pointer_acceleration_adaptive.repr
            }
            Some(_) => MirPointerAcceleration::mir_pointer_acceleration_none.repr,
            None => {
                eprintln!("Acceleration profile should be provided, but none is.");
                MirPointerAcceleration::mir_pointer_acceleration_none.repr
            }
        };

        let acceleration_bias = device_info.device.config_accel_speed();

        Box::new(PointerSettings {
            is_set: true,
            handedness,
            cursor_acceleration_bias: acceleration_bias,
            acceleration,
            horizontal_scroll_scale: guard.x_scroll_scale,
            vertical_scroll_scale: guard.y_scroll_scale,
            has_error: false,
        })
    }

    pub fn set_pointer_settings(&self, settings: &SetPointerSettingsData) {
        let Ok(mut guard) = self.state.lock() else {
            eprintln!(
                "LibinputDevice::set_pointer_settings: unable to acquire state lock; lock poisoned"
            );
            return;
        };

        let Some(device_info) = guard.find_device_by_id(self.device_id) else {
            eprintln!("Unable to set the pointer settings because the device was not found.");

            return;
        };

        if !device_info
            .device
            .has_capability(input::DeviceCapability::Pointer)
        {
            eprintln!("Device does not have the pointer capability.");
            return;
        }

        let left_handed =
            settings.handedness == MirPointerHandedness::mir_pointer_handedness_left.repr;
        let _ = device_info.device.config_left_handed_set(left_handed);

        let accel_profile = match settings.acceleration {
            x if x == MirPointerAcceleration::mir_pointer_acceleration_adaptive.repr => {
                input::AccelProfile::Adaptive
            }
            _ => input::AccelProfile::Flat,
        };
        let _ = device_info.device.config_accel_set_profile(accel_profile);
        let _ = device_info
            .device
            .config_accel_set_speed(settings.cursor_acceleration_bias);
        guard.x_scroll_scale = settings.horizontal_scroll_scale;
        guard.y_scroll_scale = settings.vertical_scroll_scale;
    }
}

pub struct LibinputDeviceState {
    pub known_devices: Vec<LibinputDeviceInfo>,
    pub next_device_id: i32,
    pub scroll_axis_x_accum: f64,
    pub scroll_axis_y_accum: f64,
    pub x_scroll_scale: f64,
    pub y_scroll_scale: f64,
}

impl LibinputDeviceState {
    pub fn find_device_by_id(&mut self, id: i32) -> Option<&mut LibinputDeviceInfo> {
        self.known_devices.iter_mut().find(|d| d.id == id)
    }
}

pub struct LibinputDeviceInfo {
    pub id: i32,
    pub device: LibinputDeviceHandle,
    pub input_device: InputDevicePtr,
    pub input_sink: Option<InputSinkPtr>,
    pub event_builder: Option<EventBuilderWrapperPtr>,
    pub button_state: u32,
    pub pointer_x: f32,
    pub pointer_y: f32,
    pub touch_properties: HashMap<u32, ContactData>,
}

#[derive(Default, Copy, Clone)]
pub struct ContactData {
    pub action: MirTouchAction,
    pub tooltype: MirTouchTooltype,
    pub x: f32,
    pub y: f32,
    pub major: f32,
    pub minor: f32,
    pub pressure: f32,
    pub orientation: f32,
    pub down_notified: bool,
}

/// Metadata about a libinput device.
///
/// This is provided to Mir upon request.
#[derive(Default)]
pub struct LibinputDeviceMetadata {
    name: String,
    unique_id: String,
    capabilities: u32,
    valid: bool,
}

impl LibinputDeviceMetadata {
    pub fn valid(&self) -> bool {
        self.valid
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn unique_id(&self) -> &str {
        &self.unique_id
    }

    pub fn capabilities(&self) -> u32 {
        self.capabilities
    }
}

pub struct LibinputDeviceObserver {
    fd: Option<i32>,
}

impl LibinputDeviceObserver {
    pub fn new() -> Self {
        LibinputDeviceObserver { fd: None }
    }

    pub fn activated(&mut self, fd: i32) {
        self.fd = Some(fd);
    }

    pub fn suspended(&mut self) {}

    pub fn removed(&mut self) {
        self.fd = None;
    }
}

// cxx's SharedPtr and UniquePtr wrapping opaque C++ types do not automatically
// implement Send or Sync, because Rust cannot verify thread-safety of the
// underlying C++ objects. However, we know that the C++ side is thread-safe,
// and all access is guarded by a Mutex. We therefore wrap the problematic types
// in newtypes and assert Send + Sync on those.

pub struct InputSinkPtr(pub std::ptr::NonNull<InputSink>);
impl InputSinkPtr {
    pub fn handle_input(&mut self, event: &cxx::SharedPtr<MirEvent>) {
        // # Safety
        //
        // Calling new_unchecked is unsafe.
        let pinned = unsafe { std::pin::Pin::new_unchecked(self.0.as_mut()) };
        pinned.handle_input(event);
    }
}

pub struct InputDevicePtr(pub cxx::SharedPtr<InputDevice>);
impl Clone for InputDevicePtr {
    fn clone(&self) -> Self {
        InputDevicePtr(self.0.clone())
    }
}
impl std::ops::Deref for InputDevicePtr {
    type Target = cxx::SharedPtr<InputDevice>;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

pub struct EventBuilderWrapperPtr(pub cxx::UniquePtr<EventBuilderWrapper>);
impl EventBuilderWrapperPtr {
    pub fn pin_mut(&mut self) -> std::pin::Pin<&mut EventBuilderWrapper> {
        self.0.pin_mut()
    }
}

/// Newtype wrapper around `input::Device` to allow asserting `Send + Sync`.
pub struct LibinputDeviceHandle(pub input::Device);
impl std::ops::Deref for LibinputDeviceHandle {
    type Target = input::Device;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}
impl std::ops::DerefMut for LibinputDeviceHandle {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

// # Safety
//
// These impls are unsafe because we are asserting that Send and Sync are valid.
// This is sound because the underlying C++ objects are thread-safe, and all
// access is serialised through the Mutex wrapping LibinputDeviceState.
unsafe impl Send for InputSinkPtr {}
unsafe impl Sync for InputSinkPtr {}
unsafe impl Send for InputDevicePtr {}
unsafe impl Sync for InputDevicePtr {}
unsafe impl Send for EventBuilderWrapperPtr {}
unsafe impl Sync for EventBuilderWrapperPtr {}
unsafe impl Send for LibinputDeviceHandle {}
unsafe impl Sync for LibinputDeviceHandle {}
