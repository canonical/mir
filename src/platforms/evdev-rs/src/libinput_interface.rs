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

use crate::ffi::bridge;
use cxx;
use input;
use libc;
use nix::unistd;
use std::os::fd::{AsRawFd, FromRawFd};
use std::os::unix::ffi::OsStrExt;
use std::os::unix::io;

pub struct LibinputInterfaceImpl {
    /// The bridge used to acquire the device.
    pub bridge: cxx::SharedPtr<bridge::PlatformBridge>,

    /// The list of opened devices.
    ///
    /// Mir expects the caller who acquired the device to keep it alive until the device
    /// is closed. To do this, we keep it in the vector.
    pub fds: Vec<cxx::UniquePtr<bridge::DeviceWrapper>>,
}

impl input::LibinputInterface for LibinputInterfaceImpl {
    // This method is called whenever libinput needs to open a new device.
    fn open_restricted(&mut self, path: &std::path::Path, _: i32) -> Result<io::OwnedFd, i32> {
        let cpath = std::ffi::CString::new(path.as_os_str().as_bytes()).unwrap();

        // # Safety
        //
        // This is an unsafe libc function.
        let mut st: libc::stat = unsafe { std::mem::zeroed() };
        let ret = unsafe { libc::stat(cpath.as_ptr(), &mut st) };
        if ret != 0 {
            return Err(ret);
        }

        let major_num = libc::major(st.st_rdev);
        let minor_num = libc::minor(st.st_rdev);

        // By keeping the device referenced in the fds vector, we ensure it stays alive
        // until close_restricted is called.
        let device = self
            .bridge
            .acquire_device(major_num as i32, minor_num as i32);
        let fd = device.raw_fd();
        self.fds.push(device);

        // # Safety
        //
        // Calling from_raw_fd is unsafe.
        let owned = unsafe { io::OwnedFd::from_raw_fd(fd) };
        Ok(owned)
    }

    // This method is called when libinput is done with a device.
    fn close_restricted(&mut self, fd: io::OwnedFd) {
        let fd_raw = fd.as_raw_fd();
        let _ = unistd::close(fd);
        self.fds.retain(|d| d.raw_fd() != fd_raw);
    }
}
