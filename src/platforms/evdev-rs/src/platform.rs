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

use crate::device::{LibinputDevice, LibinputDeviceObserver, LibinputDeviceState};
use crate::event_processing::process_libinput_events;
use crate::libinput_interface::LibinputInterfaceImpl;
use cxx;
use input;
use std::os::fd::AsRawFd;
use std::sync::Arc;
use std::sync::Mutex;

pub struct PlatformRs {
    /// The platform bridge provides access to functionality that must happen in C++.
    bridge: cxx::SharedPtr<crate::PlatformBridge>,

    /// The input device registry is used for registering and unregistering input devices.
    device_registry: cxx::SharedPtr<crate::InputDeviceRegistry>,

    report: cxx::UniquePtr<crate::InputReport>,

    state: Option<Arc<Mutex<LibinputDeviceState>>>,
}

impl PlatformRs {
    pub fn new(
        bridge: cxx::SharedPtr<crate::PlatformBridge>,
        device_registry: cxx::SharedPtr<crate::InputDeviceRegistry>,
        report: cxx::UniquePtr<crate::InputReport>,
    ) -> Self {
        PlatformRs {
            bridge,
            device_registry,
            report,
            state: None,
        }
    }

    /// Start the input platform.
    ///
    /// This method will spawn a thread to handle input events from both
    /// libinput and Mir. The thread will run until `stop()` is called.
    pub fn start(&mut self) {
        if self.state.is_some() {
            // Already started; stop() was not called before start(). This is a
            // programming error, but we handle it gracefully by doing nothing rather
            // than creating a second libinput context and leaking the existing one.
            eprintln!("evdev-rs: start() called while already started; ignoring");
            return;
        }

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

        self.state = Some(Arc::new(Mutex::new(LibinputDeviceState {
            libinput: libinput,
            known_devices: Vec::new(),
            next_device_id: 0,
            scroll_axis_x_accum: 0.0,
            scroll_axis_y_accum: 0.0,
            x_scroll_scale: 1.0,
            y_scroll_scale: 1.0,
        })));

        // Eagerly drain the DEVICE_ADDED events that libinput pre-queued during
        // udev_assign_seat. These events are held in libinput's internal buffer and
        // do NOT make the epoll fd readable. Without this drain, they would only
        // be processed when the first real input event wakes the fd watcher, causing
        // a race: the initial key-down arrives in the same dispatch batch as the
        // DEVICE_ADDED, but the device's event_builder is not yet set (the
        // registration thread hasn't run), so the event is silently dropped.
        //
        // We collect the JoinHandles from the registration threads, then release the
        // state lock BEFORE joining. The spawned threads need that same lock (inside
        // LibinputDevice::start), so we must not hold it while waiting for them.
        let handles = match self.state.as_mut().unwrap().lock() {
            Ok(mut state) => process_libinput_events(
                &mut *state,
                self.device_registry.clone(),
                self.bridge.clone(),
                &self.report,
            ),
            Err(_) => {
                eprintln!(
                    "evdev-rs platform: LibinputDeviceState mutex poisoned during startup; \
                      aborting start()"
                );
                return;
            }
        };

        // State lock is released above; now safe to join the registration threads.
        for handle in handles {
            if let Err(err) = handle.join() {
                // Log panics in registration threads so startup issues are visible.
                if let Some(message) = err.downcast_ref::<&str>() {
                    eprintln!(
                        "evdev-rs platform: device registration thread panicked with message: {}",
                        message
                    );
                } else if let Some(message) = err.downcast_ref::<String>() {
                    eprintln!(
                        "evdev-rs platform: device registration thread panicked with message: {}",
                        message
                    );
                } else {
                    eprintln!(
                         "evdev-rs platform: device registration thread panicked with non-string payload"
                     );
                }
            }
        }
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
        println!("Stopping the evdev-rs platform");
        // Note: When the main loop exits, we need to remove all devices from the device registry
        // or else Mir will try to free the devices later but we won't have access to the platform
        // code at that point. This results in a segfault.
        let Some(state_arc) = self.state.take() else {
            // Platform not started or already stopped.
            return;
        };

        // Collect the input devices to remove while holding the state lock.
        // We must NOT hold the state lock while calling remove_device(), because that
        // can cause an ABBA deadlock:
        //   - This thread: holds Rust state mutex, waiting for InputDeviceHub mutex (in remove_device)
        //   - A spawned add_device thread: holds InputDeviceHub mutex, waiting for Rust state mutex
        //     (in LibinputDevice::start)
        let devices_to_remove: Vec<cxx::SharedPtr<crate::InputDevice>> = {
            match state_arc.lock() {
                Ok(mut state_guard) => state_guard
                    .known_devices
                    .drain(..)
                    .map(|info| info.input_device)
                    .collect(),
                Err(_) => {
                    println!("Unable to gain access to device info; lock poisoned");
                    return;
                }
            }
        };

        // Remove devices from the registry WITHOUT holding the Rust state mutex.
        for input_device in &devices_to_remove {
            // # Safety
            //
            // Because we need to use pin_mut_unchecked, this is unsafe.
            unsafe {
                self.device_registry
                    .clone()
                    .pin_mut_unchecked()
                    .remove_device(input_device);
            }
        }
    }

    pub fn create_device_observer(&self) -> Box<LibinputDeviceObserver> {
        Box::new(LibinputDeviceObserver::new())
    }

    pub fn create_input_device(&mut self, device_id: i32) -> Box<LibinputDevice> {
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

                Arc::new(Mutex::new(LibinputDeviceState {
                    libinput,
                    known_devices: Vec::new(),
                    next_device_id: 0,
                    scroll_axis_x_accum: 0.0,
                    scroll_axis_y_accum: 0.0,
                    x_scroll_scale: 1.0,
                    y_scroll_scale: 1.0,
                }))
            }
        };

        Box::new(LibinputDevice {
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
                // Drop handles: runtime hotplug registration is fire-and-forget.
                let _ = process_libinput_events(
                    &mut *state,
                    self.device_registry.clone(),
                    self.bridge.clone(),
                    &self.report,
                );
            }
            _ => {}
        }
    }
}
