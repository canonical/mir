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

use std::os::fd::AsRawFd;

/// Type of udev device event.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum UdevEventType {
    Added,
    Removed,
}

/// Rust-owned udev monitor that watches for input device hotplug events.
///
/// This replaces the C++ `DispatchableUDevMonitor` class. The fd is exposed
/// to C++ which wraps it in a `ReadableFd` dispatchable.
pub struct UdevMonitor {
    socket: udev::MonitorSocket,
}

impl UdevMonitor {
    /// Create a new udev monitor filtered on the "input" subsystem.
    ///
    /// Also enumerates existing devices and calls `on_device` for each
    /// initialised input device found.
    pub fn new(mut on_device: impl FnMut(UdevEventType, &str, u64, &str)) -> Option<Self> {
        // Enumerate existing devices first.
        let mut enumerator = match udev::Enumerator::new() {
            Ok(e) => e,
            Err(e) => {
                eprintln!("evdev-rs: failed to create udev enumerator: {}", e);
                return None;
            }
        };

        if let Err(e) = enumerator.match_subsystem("input") {
            eprintln!("evdev-rs: failed to filter udev enumerator: {}", e);
            return None;
        }

        let devices = match enumerator.scan_devices() {
            Ok(d) => d,
            Err(e) => {
                eprintln!("evdev-rs: failed to scan udev devices: {}", e);
                return None;
            }
        };

        for device in devices {
            if !device.is_initialized() {
                continue;
            }

            let Some(devnode) = device.devnode() else {
                continue;
            };
            let Some(devnode_str) = devnode.to_str() else {
                continue;
            };
            let Some(sysname) = device.sysname().to_str() else {
                continue;
            };

            let devnum = device.devnum().unwrap_or(0);
            if devnum == 0 {
                continue;
            }

            on_device(UdevEventType::Added, devnode_str, devnum, sysname);
        }

        // Create the monitor.
        let socket = match udev::MonitorBuilder::new() {
            Ok(builder) => match builder.match_subsystem("input") {
                Ok(builder) => match builder.listen() {
                    Ok(socket) => socket,
                    Err(e) => {
                        eprintln!("evdev-rs: failed to listen on udev monitor: {}", e);
                        return None;
                    }
                },
                Err(e) => {
                    eprintln!("evdev-rs: failed to filter udev monitor: {}", e);
                    return None;
                }
            },
            Err(e) => {
                eprintln!("evdev-rs: failed to create udev monitor: {}", e);
                return None;
            }
        };

        Some(Self { socket })
    }

    /// Returns the file descriptor for the udev monitor socket.
    /// C++ wraps this in a `ReadableFd` dispatchable.
    pub fn fd(&self) -> i32 {
        self.socket.as_raw_fd()
    }

    /// Process pending udev events and call the handler for each.
    pub fn process_events(&mut self, mut on_event: impl FnMut(UdevEventType, &str, u64, &str)) {
        // The udev crate's MonitorSocket implements Iterator for events.
        // We need to use iter() which is non-blocking when the fd is readable.
        while let Some(event) = self.socket.iter().next() {
            let event_type = match event.event_type() {
                udev::EventType::Add => UdevEventType::Added,
                udev::EventType::Remove => UdevEventType::Removed,
                _ => continue,
            };

            // Re-fetch via syspath to work around umockdev missing properties.
            let device = match udev::Device::from_syspath(event.syspath()) {
                Ok(d) => d,
                Err(_) => continue,
            };

            let Some(devnode) = device.devnode() else {
                continue;
            };
            let Some(devnode_str) = devnode.to_str() else {
                continue;
            };
            let Some(sysname) = device.sysname().to_str() else {
                continue;
            };

            let devnum = device.devnum().unwrap_or(0);
            if devnum == 0 {
                continue;
            }

            on_event(event_type, devnode_str, devnum, sysname);
        }
    }
}
