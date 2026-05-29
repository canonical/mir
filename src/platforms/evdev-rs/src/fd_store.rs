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
use std::os::fd::RawFd;

/// Manages pre-acquired and backup file descriptors for device nodes.
///
/// When a device is activated via ConsoleServices, its fd is stored here
/// so that libinput's `open_restricted` callback can claim it without
/// blocking. A dup'd backup is kept so that libinput can re-open devices
/// it has internally closed (e.g. after a lid switch event).
pub struct FdStore {
    pending_fds: HashMap<String, RawFd>,
    backup_fds: HashMap<String, RawFd>,
}

impl FdStore {
    pub fn new() -> Self {
        FdStore {
            pending_fds: HashMap::new(),
            backup_fds: HashMap::new(),
        }
    }

    /// Store a pre-acquired file descriptor for a device path.
    /// Also creates a dup'd backup for future re-opens.
    pub fn store_pending(&mut self, devnode: &str, fd: RawFd) {
        self.pending_fds.insert(devnode.to_string(), fd);

        // Keep a dup'd backup so libinput can re-open the device later
        // (e.g. after handling a lid switch event internally).
        let backup = unsafe { libc::dup(fd) };
        if backup >= 0 {
            // Close any previous backup for this device.
            if let Some(old_backup) = self.backup_fds.insert(devnode.to_string(), backup) {
                unsafe { libc::close(old_backup) };
            }
        }
    }

    /// Claim and remove the pre-acquired fd for a device path.
    /// Returns the raw fd, or -1 if no pending fd exists.
    pub fn claim_pending(&mut self, devnode: &str) -> RawFd {
        self.pending_fds.remove(devnode).unwrap_or(-1)
    }

    /// Claim a dup'd backup fd for a device path.
    /// Used when libinput internally re-opens a device (e.g. after lid switch).
    /// Returns a dup'd fd, or -1 if no backup exists.
    pub fn claim_backup(&self, devnode: &str) -> RawFd {
        match self.backup_fds.get(devnode) {
            Some(&backup_fd) => {
                // Return a dup so the backup remains available for future re-opens.
                let duped = unsafe { libc::dup(backup_fd) };
                duped
            }
            None => -1,
        }
    }

    /// Remove the backup fd for a device (called on device removal).
    pub fn remove_backup(&mut self, devnode: &str) {
        if let Some(fd) = self.backup_fds.remove(devnode) {
            unsafe { libc::close(fd) };
        }
    }
}

impl Drop for FdStore {
    fn drop(&mut self) {
        for (_, fd) in self.backup_fds.drain() {
            unsafe { libc::close(fd) };
        }
        // pending_fds are not closed here — ownership was transferred to the caller
        // (libinput) when they were claimed via claim_pending. Any unclaimed pending
        // fds represent a logic error but we don't close them because they may still
        // be referenced by the ConsoleServices Device handle.
    }
}
