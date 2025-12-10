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

use crate::device::{DeviceObserverRs, InputDeviceRs, LibinputLoopState};
use crate::event_processing::process_libinput_events;
use crate::libinput_interface::LibinputInterfaceImpl;
use cxx;
use input;
use input::AsRaw;
use std::os::fd::AsRawFd;
use std::sync::Arc;
use std::sync::Mutex;

pub struct PlatformRs {
    /// The platform bridge provides access to functionality that must happen in C++.
    bridge: cxx::SharedPtr<crate::PlatformBridge>,

    /// The input device registry is used for registering and unregistering input devices.
    device_registry: cxx::SharedPtr<crate::InputDeviceRegistry>,

    state: Option<Arc<Mutex<LibinputLoopState>>>,
}

impl PlatformRs {
    pub fn new(
        bridge: cxx::SharedPtr<crate::PlatformBridge>,
        device_registry: cxx::SharedPtr<crate::InputDeviceRegistry>,
    ) -> Self {
        PlatformRs {
            bridge,
            device_registry,
            state: None,
        }
    }

    /// Start the input platform.
    ///
    /// This method will spawn a thread to handle input events from both
    /// libinput and Mir. The thread will run until `stop()` is called.
    pub fn start(&mut self) {
        println!("Starting the evdev-rs platform");

        let bridge = self.bridge.clone();
        let mut libinput = input::Libinput::new_with_udev(LibinputInterfaceImpl {
            bridge: bridge.clone(),
            fds: Vec::new(),
        });

        // TODO: This does not handle multi-seat. If/when we do multi-seat, this will need to be refactored.
        let started = match libinput.udev_assign_seat("seat0") {
            Err(_) => false,
            _ => true,
        };

        if !started {
            eprintln!("Failed to call udev_assign_seat, not running the libinput loop");
            return;
        }

        self.state = Some(Arc::new(Mutex::new(LibinputLoopState {
            libinput: libinput,
            known_devices: Vec::new(),
            next_device_id: 0,
            button_state: 0,
            pointer_x: 0.0,
            pointer_y: 0.0,
            scroll_axis_x_accum: 0.0,
            scroll_axis_y_accum: 0.0,
            x_scroll_scale: 1.0,
            y_scroll_scale: 1.0,
        })));
    }

    pub unsafe fn libinput_fd(&mut self) -> i32 {
        match self.state.as_mut() {
            Some(state_arc) => match state_arc.lock() {
                Ok(state) => state.libinput.as_raw_fd(),
                Err(_) => -1,
            },
            None => -1,
        }
    }

    pub fn continue_after_config(&self) {}
    pub fn pause_for_config(&self) {}

    /// Stop the input platform.
    ///
    /// This method will signal the input thread to stop and wait for it to finish.
    pub fn stop(&mut self) {
        // Note: When the main loop exits, we need to remove all devices from the device registry
        // or else Mir will try to free the devices later but we won't have access to the platform
        // code at that point. This results in a segfault.
        let state_opt = self.state.as_mut();
        if state_opt.is_none() {
            // Platform not started or already stopped.
            self.state = None;
            return;
        }

        match state_opt.unwrap().lock() {
            Ok(mut state_guard) => {
                for device_info in state_guard.known_devices.iter_mut().rev() {
                    println!("Removing device id {} from registry", device_info.id);
                    device_info.input_sink = None;
                    device_info.event_builder = None;
                    // # Safety
                    //
                    // Because we need to use pin_mut_unchecked, this is unsafe.
                    unsafe {
                        self.device_registry
                            .clone()
                            .pin_mut_unchecked()
                            .remove_device(&device_info.input_device);
                    }
                }
            }
            Err(_) => {
                println!("Unable to gain access to device info; lock poisoned");
            }
        }

        self.state = None;
    }

    pub fn create_device_observer(&self) -> Box<DeviceObserverRs> {
        Box::new(DeviceObserverRs::new())
    }

    pub fn create_input_device(&mut self, device_id: i32) -> Box<InputDeviceRs> {
        let state_arc = match self.state.as_mut() {
            Some(s) => s.clone(),
            None => {
                println!(
                    "PlatformRs::create_input_device: state not initialized; creating fallback state"
                );
                let mut libinput = input::Libinput::new_with_udev(LibinputInterfaceImpl {
                    bridge: self.bridge.clone(),
                    fds: Vec::new(),
                });

                let started = match libinput.udev_assign_seat("seat0") {
                    Err(_) => false,
                    _ => true,
                };
                if !started {
                    println!(
                        "PlatformRs::create_input_device: failed to assign seat for fallback state"
                    );
                }

                Arc::new(Mutex::new(LibinputLoopState {
                    libinput,
                    known_devices: Vec::new(),
                    next_device_id: 0,
                    button_state: 0,
                    pointer_x: 0.0,
                    pointer_y: 0.0,
                    scroll_axis_x_accum: 0.0,
                    scroll_axis_y_accum: 0.0,
                    x_scroll_scale: 1.0,
                    y_scroll_scale: 1.0,
                }))
            }
        };

        Box::new(InputDeviceRs {
            device_id,
            state: state_arc,
            bridge: self.bridge.clone(),
        })
    }

    pub fn process(&mut self) {
        if self.state.is_none() {
            return;
        }

        match self.state.as_mut().unwrap().try_lock() {
            Ok(mut state) => {
                process_libinput_events(
                    &mut *state,
                    self.device_registry.clone(),
                    self.bridge.clone(),
                );
            }
            _ => {}
        }
    }
}
