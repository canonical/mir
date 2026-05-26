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
    /// Called by libinput (synchronously inside `path_add_device`) to open a device.
    ///
    /// Rather than acquiring the device here (which would require a blocking logind
    /// D-Bus call), we retrieve a file descriptor that was pre-acquired asynchronously
    /// by the C++ `Device::Observer::activated()` callback and stored in the bridge's
    /// pending-fd map.
    fn open_restricted(&mut self, path: &std::path::Path, _flags: i32) -> Result<io::OwnedFd, i32> {
        let path_str = path.to_str().ok_or(-libc::EINVAL)?;
        let raw_fd = self.bridge.claim_pending_fd(path_str);
        if raw_fd < 0 {
            eprintln!(
                "evdev-rs open_restricted: no pre-acquired fd for {}",
                path_str
            );
            return Err(-libc::ENOENT);
        }

        // Safety: claim_pending_fd transferred ownership of this fd to us.
        let owned = unsafe { io::OwnedFd::from_raw_fd(raw_fd) };
        Ok(owned)
    }

    /// Called by libinput when it is done with a device fd.
    ///
    /// The `OwnedFd` closes the file descriptor automatically when dropped.
    fn close_restricted(&mut self, _fd: io::OwnedFd) {}
}
