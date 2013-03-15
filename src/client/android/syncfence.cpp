/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "syncfence.h"

namespace mcla = mir::client::android;

mcla::SyncFence::SyncFence(int fd, std::shared_ptr<IoctlWrapper> const& wrapper)
 : ioctl_wrapper(wrapper),
   fence_fd(fd)
{
}

mcla::SyncFence::~SyncFence()
{
    if (fence_fd > 0)
    {
        ioctl_wrapper->close(fence_fd);
    }
}

void mcla::SyncFence::wait()
{
    if (fence_fd > 0)
    {
        int timeout = -1;
        ioctl_wrapper->ioctl(fence_fd, SYNC_IOC_WAIT, &timeout);
    }
}
