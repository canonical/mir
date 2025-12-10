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

// TODO: Report errors to Mir's logging facilities. We should do this following Mir's logging refactor.
// TODO: Need to set up reporting events when received from libinput (report->received_event_from_kernel)
// TODO: Implement continue after_config and pause_for_config

// Some notes about the implementation here:
// 1. CXX-Rust doesn't support passing Option<T> to C++ functions, so I use booleans
//    instead.
// 2. All enums are changed to i32 for ABI stability

mod device;
mod event_processing;
pub mod ffi;
mod libinput_interface;
mod platform;

use crate::device::{DeviceObserverRs, InputDeviceInfoRs, InputDeviceRs};
use crate::platform::PlatformRs;

/// Creates a platform on the Rust side of things.
///
/// The `bridge` and `device_registry` parameters are provided by the C++ side of things.
/// The C++ platform simply forwards all calls to the Rust implementation.
pub fn evdev_rs_create(
    bridge: cxx::SharedPtr<ffi::bridge::PlatformBridge>,
    device_registry: cxx::SharedPtr<ffi::bridge::InputDeviceRegistry>,
) -> Box<PlatformRs> {
    return Box::new(PlatformRs::new(bridge, device_registry));
}

// # Safety
//
// The other side of the classes below are known by us to be thread-safe, so we can
// safely send it to another thread. However, Rust has no way of knowing that, so
// we have to assert it ourselves.
unsafe impl Send for ffi::bridge::PlatformBridge {}
unsafe impl Sync for ffi::bridge::PlatformBridge {}
unsafe impl Send for ffi::bridge::InputDeviceRegistry {}
unsafe impl Sync for ffi::bridge::InputDeviceRegistry {}
unsafe impl Send for ffi::bridge::InputDevice {}
unsafe impl Sync for ffi::bridge::InputDevice {}
unsafe impl Send for ffi::bridge::InputSink {}
unsafe impl Sync for ffi::bridge::InputSink {}
unsafe impl Send for ffi::bridge::EventBuilder {}
unsafe impl Sync for ffi::bridge::EventBuilder {}

