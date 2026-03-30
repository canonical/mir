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
use crate::event_processing::{
    drain_initial_events, process_deferred_events, process_libinput_events,
};
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
    ///
    /// Note: this only creates the libinput context and does NOT call
    /// `udev_assign_seat()`.  Seat assignment (and therefore device opening)
    /// is deferred to `assign_seat()`, which must be called after this
    /// function returns so that the GLib main loop is free to process the
    /// logind `TakeDevice` D-Bus replies that opening each device requires.
    ///
    /// Returns `true` if a fresh context was created, `false` if the platform
    /// was already started.  The caller must only schedule `assign_seat()` when
    /// this returns `true` to avoid calling `udev_assign_seat()` on an already-
    /// assigned libinput context.
    pub fn start(&mut self) -> bool {
        if self.state.is_some() {
            // Already started; stop() was not called before start(). This is a
            // programming error, but we handle it gracefully by doing nothing rather
            // than creating a second libinput context and leaking the existing one.
            return false;
        }

        println!("Starting the evdev-rs platform");

        let bridge = self.bridge.clone();
        let libinput = input::Libinput::new_with_udev(LibinputInterfaceImpl {
            bridge: bridge.clone(),
            fds: Vec::new(),
        });

        self.state = Some(Arc::new(Mutex::new(LibinputDeviceState {
            libinput,
            known_devices: Vec::new(),
            next_device_id: 0,
            scroll_axis_x_accum: 0.0,
            scroll_axis_y_accum: 0.0,
            x_scroll_scale: 1.0,
            y_scroll_scale: 1.0,
        })));

        true
    }

    /// Assign a udev seat so that libinput begins opening input devices.
    ///
    /// This must be called after `start()` **and** after `start_platforms()`
    /// has returned to the caller, so that the GLib main loop is no longer
    /// blocked and can process the logind `TakeDevice` D-Bus replies that
    /// opening each input device requires.
    ///
    /// After `udev_assign_seat()` returns, `libinput_dispatch()` must be called
    /// to make the `DEVICE_ADDED` events accessible.  Any input events buffered
    /// in the kernel device fds while devices were being opened (e.g. a modifier
    /// key held during a logind `TakeDevice` round-trip) also become available in
    /// that same `dispatch()` call.  We split these two kinds of events:
    ///
    /// - `DEVICE_ADDED` events are processed immediately (registration threads
    ///   spawned) while the state lock is held.
    /// - Non-device input events are deferred to a Vec.
    ///
    /// The state lock is then released, registration threads are joined (so
    /// `event_builder` is set for all devices), and only then the deferred input
    /// events are processed.  This prevents the #4723 drop race where a buffered
    /// key event would otherwise be processed in the same batch as `DEVICE_ADDED`
    /// before `event_builder` is set.
    ///
    /// # Safety
    /// This function calls into C++ (via `acquire_device`) while holding the
    /// Rust state mutex, but no other Rust code that also needs the state
    /// mutex can run concurrently on the input thread at this point.
    pub fn assign_seat(&mut self) {
        // Clone the Arc so we are not borrowing from self.state — this allows
        // us to call self.state.take() on failure paths below without a
        // borrow-check conflict.
        let state_arc = match &self.state {
            Some(arc) => arc.clone(),
            None => {
                return;
            }
        };

        // Phase 1: assign the seat, dispatch once to surface DEVICE_ADDED events,
        // and split the resulting queue into device events (handled immediately)
        // and deferred input events.
        //
        // The state lock is held for the entire phase.  Registration threads are
        // spawned but not yet joined — they need the same lock (inside
        // LibinputDevice::start) so we must release it before joining.
        // TODO: This does not handle multi-seat. If/when we do multi-seat, this will need to be refactored.
        let phase1_result = match state_arc.lock() {
            Ok(mut state) => {
                // Note: udev_assign_seat returns Result<(), ()> — no error details are available.
                if state.libinput.udev_assign_seat("seat0").is_err() {
                    eprintln!("Failed to call udev_assign_seat, not running the libinput loop");
                    None
                } else {
                    Some(drain_initial_events(
                        &mut *state,
                        self.device_registry.clone(),
                        self.bridge.clone(),
                    ))
                }
            }
            Err(_) => {
                // A poisoned mutex means another thread panicked while holding it,
                // leaving the state in an unknown condition.
                None
            }
        };
        // State lock is released here.

        let (handles, deferred) = match phase1_result {
            Some(result) => result,
            None => {
                // On any failure, reset the platform state so that a subsequent
                // start() call can create a fresh libinput context and retry.
                self.state.take();
                return;
            }
        };

        // Phase 2: join registration threads so that event_builder is set for
        // all devices before we process any input events.
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

        // Phase 3: process any input events (keyboard, pointer, …) that arrived
        // in the same dispatch() batch as DEVICE_ADDED.  Now that event_builder
        // is set these events will be handled correctly rather than dropped.
        if !deferred.is_empty() {
            match state_arc.lock() {
                Ok(mut state) => {
                    process_deferred_events(&mut *state, &self.bridge, deferred, &self.report);
                }
                Err(_) => {
                    self.state.take();
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
                Err(poisoned) => {
                    // The mutex was poisoned by a previous panic, but we still drain
                    // known_devices from the inner state to avoid leaving them registered
                    // in the InputDeviceRegistry, which can lead to segfaults later.
                    let mut state_guard = poisoned.into_inner();
                    state_guard
                        .known_devices
                        .drain(..)
                        .map(|info| info.input_device)
                        .collect()
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
