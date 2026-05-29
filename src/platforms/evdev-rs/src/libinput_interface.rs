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

use cxx;
use input;
use libc;
use std::os::fd::FromRawFd;
use std::os::unix::io;

pub struct LibinputInterfaceImpl {
    /// The bridge used to claim pre-acquired file descriptors.
    pub bridge: cxx::SharedPtr<crate::PlatformBridge>,
}

impl input::LibinputInterface for LibinputInterfaceImpl {
    /// Called by libinput to open a device.
    ///
    /// First tries the pre-acquired fd (stored during `path_add_device` flow).
    /// If that's not available (e.g. libinput internally re-opens a device after
    /// handling a lid switch event), falls back to a dup'd backup fd.
    fn open_restricted(&mut self, path: &std::path::Path, _flags: i32) -> Result<io::OwnedFd, i32> {
        let path_str = path.to_str().ok_or(-libc::EINVAL)?;
        let raw_fd = self.bridge.claim_pending_fd(path_str);
        if raw_fd >= 0 {
            // Safety: claim_pending_fd transferred ownership of this fd to us.
            let owned = unsafe { io::OwnedFd::from_raw_fd(raw_fd) };
            return Ok(owned);
        }

        // Fallback: libinput is re-opening a device it previously closed internally
        // (e.g. after a lid switch event). Use the backup fd.
        let backup_fd = self.bridge.claim_backup_fd(path_str);
        if backup_fd >= 0 {
            println!("evdev-rs open_restricted: using backup fd for {}", path_str);
            let owned = unsafe { io::OwnedFd::from_raw_fd(backup_fd) };
            return Ok(owned);
        }

        println!(
            "evdev-rs open_restricted: no pre-acquired or backup fd for {}",
            path_str
        );
        Err(-libc::ENOENT)
    }

    /// Called by libinput when it is done with a device fd.
    ///
    /// The `OwnedFd` closes the file descriptor automatically when dropped.
    fn close_restricted(&mut self, fd: io::OwnedFd) {
        // We handle FD externally, from UDev/the ConsoleProvider
        std::mem::forget(fd);
    }
}
