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

use crate::device::{LibinputDevice, LibinputDeviceState, ScrollState};
use crate::event_processing::process_libinput_events;
use crate::fd_store::FdStore;
use crate::udev_monitor::{UdevEventType, UdevMonitor};
use std::collections::HashSet;
use std::os::fd::{AsRawFd, FromRawFd, OwnedFd};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::sync::Mutex;

pub struct PlatformRs {
    /// The platform bridge provides access to functionality that must happen in C++.
    bridge: cxx::SharedPtr<crate::PlatformBridge>,

    /// The input device registry is used for registering and unregistering input devices.
    device_registry: cxx::SharedPtr<crate::InputDeviceRegistry>,

    report: cxx::UniquePtr<crate::InputReport>,

    state: Option<Arc<Mutex<LibinputDeviceState>>>,

    /// Whether the platform is currently running. Owned by Rust, queried by C++.
    running: Arc<AtomicBool>,

    /// Set of devnums we know about (pending or active), to deduplicate udev events.
    known_devnums: HashSet<u64>,

    /// Rust-owned udev monitor for input device hotplug events.
    udev_monitor: Option<UdevMonitor>,

    /// Manages pre-acquired and backup file descriptors for device nodes.
    fd_store: Arc<Mutex<FdStore>>,
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
            running: Arc::new(AtomicBool::new(false)),
            known_devnums: HashSet::new(),
            udev_monitor: None,
            fd_store: Arc::new(Mutex::new(FdStore::new())),
        }
    }

    /// Start the input platform.
    ///
    /// Creates the libinput context using the path-based API so that device
    /// acquisition (via `path_add_device`) is driven by the `UdevMonitor`.
    /// This avoids `udev_assign_seat()`, which would call `open_restricted()`
    /// synchronously while the GLib main loop still needs to process the logind
    /// `TakeDevice` D-Bus replies.
    ///
    /// Returns `true` if a fresh context was created, `false` if the platform
    /// was already started.
    pub fn start(&mut self) -> bool {
        if self.state.is_some() {
            return false;
        }

        println!("Starting the evdev-rs platform");

        let libinput =
            input::Libinput::new_from_path(crate::libinput_interface::LibinputInterfaceImpl {
                fd_store: self.fd_store.clone(),
            });

        self.state = Some(Arc::new(Mutex::new(LibinputDeviceState {
            libinput,
            known_devices: Vec::new(),
            next_device_id: 0,
            scroll_state: ScrollState {
                x_accum: 0.0,
                y_accum: 0.0,
                x_scroll_scale: 1.0,
                y_scroll_scale: 1.0,
            },
        })));

        self.running.store(true, Ordering::Release);
        self.known_devnums.clear();

        // Create the udev monitor. The closure handles initial device enumeration.
        let mut initial_devices: Vec<(String, u64, String)> = Vec::new();
        self.udev_monitor = UdevMonitor::new(|_event_type, devnode, devnum, sysname| {
            initial_devices.push((devnode.to_string(), devnum, sysname.to_string()));
        });

        // Process the initially-enumerated devices through on_udev_event.
        for (devnode, devnum, sysname) in initial_devices {
            self.on_udev_event(UdevEventType::Added, &devnode, devnum, &sysname);
        }

        true
    }

    /// Add a device to the libinput context by path.
    ///
    /// Called internally after a device fd has been acquired and stored in
    /// the bridge's pending-fd map. libinput will call `open_restricted()`,
    /// which retrieves the pre-acquired fd non-blockingly via `claim_pending_fd`.
    pub fn path_add_device(&mut self, devnode: &str) {
        let Some(state_arc) = self.state.as_mut() else {
            println!("PlatformRs::path_add_device: platform not started");
            return;
        };
        match state_arc.lock() {
            Ok(mut state) => {
                println!("evdev-rs: libinput path_add_device({})", devnode);
                state.libinput.path_add_device(devnode);
            }
            Err(_) => {
                println!("PlatformRs::path_add_device: state mutex poisoned");
            }
        }
    }

    /// Remove a device from the libinput context by path.
    ///
    /// Called internally when a device is suspended (VT switch) or
    /// removed (hotplug disconnect).
    pub fn path_remove_device(&mut self, devnode: &str) {
        let Some(state_arc) = self.state.as_mut() else {
            return;
        };
        match state_arc.lock() {
            Ok(mut state) => {
                let index = state
                    .known_devices
                    .iter()
                    .position(|d| d.devnode == devnode);
                let Some(index) = index else {
                    println!(
                        "evdev-rs: path_remove_device({}) — not in known_devices",
                        devnode
                    );
                    return;
                };
                println!(
                    "evdev-rs: path_remove_device({}) — removing from libinput",
                    devnode
                );
                let device_info = state.known_devices.swap_remove(index);
                state.libinput.path_remove_device(device_info.device);

                let input_device = device_info.input_device.clone();
                let registry = self.device_registry.clone();
                std::thread::spawn(move || unsafe {
                    registry
                        .clone()
                        .pin_mut_unchecked()
                        .remove_device(&input_device);
                });
            }
            Err(_) => {
                println!("PlatformRs::path_remove_device: state mutex poisoned");
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
        self.running.store(false, Ordering::Release);
        self.known_devnums.clear();
        self.udev_monitor = None;

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

    pub fn create_input_device(&mut self, device_id: i32) -> Box<LibinputDevice> {
        let state_arc = match self.state.as_mut() {
            Some(s) => s.clone(),
            None => {
                eprintln!(
                    "PlatformRs::create_input_device: platform not started (device_id={})",
                    device_id
                );
                // Return a device that will fail gracefully when used.
                return Box::new(LibinputDevice {
                    device_id,
                    state: Arc::new(Mutex::new(LibinputDeviceState {
                        libinput: input::Libinput::new_from_path(
                            crate::libinput_interface::LibinputInterfaceImpl {
                                fd_store: self.fd_store.clone(),
                            },
                        ),
                        known_devices: Vec::new(),
                        next_device_id: 0,
                        scroll_state: ScrollState {
                            x_accum: 0.0,
                            y_accum: 0.0,
                            x_scroll_scale: 1.0,
                            y_scroll_scale: 1.0,
                        },
                    })),
                    bridge: self.bridge.clone(),
                });
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

        if let Ok(mut state) = self.state.as_mut().unwrap().try_lock() {
            process_libinput_events(
                &mut state,
                self.device_registry.clone(),
                self.bridge.clone(),
                &self.report,
            );
        }
    }

    /// Returns whether the platform is currently running.
    /// Queried from C++ (InputDeviceObserver) and internally.
    pub fn is_running(&self) -> bool {
        self.running.load(Ordering::Acquire)
    }

    /// Returns the file descriptor for the udev monitor.
    /// C++ wraps this in a `ReadableFd` dispatchable.
    /// Returns -1 if the monitor is not available.
    pub fn udev_monitor_fd(&self) -> i32 {
        match &self.udev_monitor {
            Some(monitor) => monitor.fd(),
            None => -1,
        }
    }

    /// Process pending udev monitor events.
    /// Invoked from C++ when the udev monitor fd becomes readable.
    pub fn process_udev_events(&mut self) {
        if !self.running.load(Ordering::Acquire) {
            return;
        }

        // Take the monitor out temporarily to avoid borrow issues with self.
        let Some(mut monitor) = self.udev_monitor.take() else {
            return;
        };

        let mut events: Vec<(UdevEventType, String, u64, String)> = Vec::new();
        monitor.process_events(|event_type, devnode, devnum, sysname| {
            events.push((event_type, devnode.to_string(), devnum, sysname.to_string()));
        });

        // Put the monitor back before processing events (which need &mut self).
        self.udev_monitor = Some(monitor);

        for (event_type, devnode, devnum, sysname) in events {
            self.on_udev_event(event_type, &devnode, devnum, &sysname);
        }
    }

    /// Handle a udev event. Decides whether to acquire or release a device.
    ///
    /// Called internally by `process_udev_events()` (at runtime when the
    /// udev fd is readable) and by `start()` (during initial enumeration).
    pub fn on_udev_event(
        &mut self,
        event_type: UdevEventType,
        devnode: &str,
        devnum: u64,
        sysname: &str,
    ) {
        if !self.running.load(Ordering::Acquire) {
            return;
        }

        match event_type {
            UdevEventType::Added => {
                // Only process "event" devices (what libinput expects).
                if !sysname.starts_with("event") {
                    return;
                }

                // Deduplicate: skip if already pending or active.
                if self.known_devnums.contains(&devnum) {
                    println!(
                        "evdev-rs: on_udev_event Added devnode={} devnum={} — already known, skipping",
                        devnode, devnum
                    );
                    return;
                }

                println!(
                    "evdev-rs: on_udev_event Added devnode={} devnum={} — acquiring",
                    devnode, devnum
                );

                // Ask C++ to initiate device acquisition via ConsoleServices.
                if self.bridge.acquire_device(devnum, devnode) {
                    self.known_devnums.insert(devnum);
                }
            }
            UdevEventType::Removed => {
                println!(
                    "evdev-rs: on_udev_event Removed devnode={} devnum={}",
                    devnode, devnum
                );

                if !self.known_devnums.remove(&devnum) {
                    println!("evdev-rs: on_udev_event Removed — not in known_devnums, skipping");
                    return;
                }

                // Release the pending future if activated() never fired.
                self.bridge.release_pending_device(devnum);

                // If already active, remove from libinput and release.
                if self.bridge.has_device(devnum) {
                    if !devnode.is_empty() {
                        self.path_remove_device(devnode);
                    }
                    self.bridge.release_device(devnum);
                }

                if let Ok(mut store) = self.fd_store.lock() {
                    store.remove(devnode);
                }
            }
        }
    }

    /// Handle device activation (fd acquired from ConsoleServices).
    ///
    /// Invoked from C++ (on the input dispatch thread via ActionQueue)
    /// after `Device::Observer::activated()` has fired and the fd has
    /// been dup'd.
    pub fn on_device_activated(&mut self, devnode: &str, devnum: u64, fd: i32) {
        println!(
            "evdev-rs: on_device_activated devnode={} devnum={} fd={}",
            devnode, devnum, fd
        );

        if !self.running.load(Ordering::Acquire) {
            println!("evdev-rs: on_device_activated ignored — platform not running");
            return;
        }

        // Store the fd so that open_restricted() can claim it.
        match self.fd_store.lock() {
            Ok(mut store) => store.store(devnode, unsafe { OwnedFd::from_raw_fd(fd) }),
            Err(_) => {
                println!("evdev-rs: on_device_activated: fd_store mutex poisoned");
                return;
            }
        }

        // Transfer the Device from pending to active in C++.
        self.bridge.activate_pending_device(devnum);

        // Tell libinput about the new device.
        println!(
            "evdev-rs: on_device_activated calling path_add_device({})",
            devnode
        );
        self.path_add_device(devnode);
    }

    /// Handle device suspension (e.g. VT switch or lid close).
    ///
    /// Invoked from C++ (on the input dispatch thread via ActionQueue).
    pub fn on_device_suspended(&mut self, devnode: &str, devnum: u64) {
        println!(
            "evdev-rs: on_device_suspended devnode={} devnum={}",
            devnode, devnum
        );

        if !self.running.load(Ordering::Acquire) {
            println!("evdev-rs: on_device_suspended ignored — platform not running");
            return;
        }

        println!(
            "evdev-rs: on_device_suspended calling path_remove_device({})",
            devnode
        );

        self.bridge.release_device(devnum);
        self.path_remove_device(devnode);
    }

    /// Handle device removal (hotplug disconnect via observer).
    ///
    /// Invoked from C++ (on the input dispatch thread via ActionQueue).
    pub fn on_device_removed(&mut self, devnode: &str, devnum: u64) {
        println!(
            "evdev-rs: on_device_removed devnode={} devnum={}",
            devnode, devnum
        );

        if !self.running.load(Ordering::Acquire) {
            println!("evdev-rs: on_device_removed ignored — platform not running");
            return;
        }

        self.known_devnums.remove(&devnum);
        self.bridge.release_device(devnum);
        if let Ok(mut store) = self.fd_store.lock() {
            store.remove(devnode);
        }
        self.path_remove_device(devnode);
    }
}
