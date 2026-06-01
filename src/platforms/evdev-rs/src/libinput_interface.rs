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

use crate::fd_store::FdStore;
use input;
use libc;
use std::os::fd::FromRawFd;
use std::os::unix::io;
use std::sync::{Arc, Mutex};

pub struct LibinputInterfaceImpl {
    /// The fd store used to claim pre-acquired file descriptors.
    pub fd_store: Arc<Mutex<FdStore>>,
}

impl input::LibinputInterface for LibinputInterfaceImpl {
    /// Called by libinput to open a device.
    ///
    /// Returns a dup of the pre-acquired fd stored during `path_add_device`.
    /// The stored fd is never consumed, so libinput can re-open the device
    /// any number of times (e.g. after a lid-switch event) without needing a
    /// separate backup.
    fn open_restricted(&mut self, path: &std::path::Path, _flags: i32) -> Result<io::OwnedFd, i32> {
        let path_str = path.to_str().ok_or(-libc::EINVAL)?;

        let store = self.fd_store.lock().map_err(|_| -libc::ENOLCK)?;

        let raw_fd = store.claim_pending(path_str);
        if raw_fd >= 0 {
            // Safety: claim_pending returned a fresh dup; we own it.
            return Ok(unsafe { io::OwnedFd::from_raw_fd(raw_fd) });
        }

        println!(
            "evdev-rs open_restricted: no pre-acquired fd for {}",
            path_str
        );
        Err(-libc::ENOENT)
    }

    /// Called by libinput when it is done with a device fd.
    ///
    /// Drops the `OwnedFd`, closing the dup that was handed to libinput.
    /// The original fd remains in `FdStore` until the device is removed.
    fn close_restricted(&mut self, _fd: io::OwnedFd) {
        // OwnedFd closes the dup automatically on drop.
    }
}
