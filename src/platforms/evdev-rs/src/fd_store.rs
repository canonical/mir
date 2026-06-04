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

use std::collections::HashMap;
use std::os::fd::OwnedFd;

/// Manages pre-acquired file descriptors for device nodes.
///
/// When a device is activated via ConsoleServices, its fd is stored here
/// so that libinput's `open_restricted` callback can claim it without
/// blocking. The fd is never removed on claim — each `open_restricted`
/// call receives a fresh `dup` — so libinput can re-open the device as
/// many times as it likes (e.g. after a lid-switch event) without any
/// separate backup mechanism. The fd is only released when the device is
/// explicitly removed via `remove`.
pub struct FdStore {
    fds: HashMap<String, OwnedFd>,
}

impl FdStore {
    pub fn new() -> Self {
        FdStore {
            fds: HashMap::new(),
        }
    }

    /// Store a pre-acquired file descriptor for a device path.
    /// Any previous fd for the same device is closed automatically.
    pub fn store(&mut self, devnode: &str, fd: OwnedFd) {
        self.fds.insert(devnode.to_string(), fd);
    }

    /// Return a dup'd fd for a device path without consuming the stored fd.
    /// Returns `None` if no fd is stored for this device.
    /// The caller owns the returned fd and is responsible for closing it.
    pub fn claim(&self, devnode: &str) -> Option<OwnedFd> {
        self.fds.get(devnode)?.try_clone().ok()
    }

    /// Remove and close the stored fd for a device (called on device removal).
    pub fn remove(&mut self, devnode: &str) {
        self.fds.remove(devnode);
    }
}
