/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "syncfence.h"

namespace mga = mir::graphics::android;

mga::SyncFence::SyncFence(int fd, std::shared_ptr<IoctlWrapper> const& wrapper)
 : ioctl_wrapper(wrapper),
   fence_fd(fd)
{
}

mga::SyncFence::~SyncFence() noexcept
{
    if (fence_fd > 0)
    {
        ioctl_wrapper->close(fence_fd);
    }
}

void mga::SyncFence::wait()
{
    if (fence_fd > 0)
    {
        int timeout = -1;
        ioctl_wrapper->ioctl(fence_fd, SYNC_IOC_WAIT, &timeout);
    }
}
