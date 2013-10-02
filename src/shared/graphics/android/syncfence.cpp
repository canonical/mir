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

#include "mir/graphics/android/sync_fence.h"


//FIXME: (lp-1229884) this ioctl code should be taken from kernel headers
#define SYNC_IOC_WAIT 0x40043E00
#define SYNC_IOC_MERGE 0x444

namespace mga = mir::graphics::android;

mga::SyncFence::SyncFence(std::shared_ptr<mga::SyncFileOps> const& ops, int fd)
   : fence_fd(fd),
     ops(ops)
{
}

mga::SyncFence::~SyncFence() noexcept
{
    if (fence_fd > 0)
    {
        ops->close(fence_fd);
    }
}

void mga::SyncFence::wait()
{
    if (fence_fd > 0)
    {
        int timeout = -1;
        ops->ioctl(fence_fd, SYNC_IOC_WAIT, &timeout);
    }
}

void mga::SyncFence::merge_with(mga::Fence&& fence)
{
    if(fence_fd < 0)
    {
        fence_fd = fence.extract_native_handle(); 
    }
    else 
    {
        auto new_fence_fd = ops->ioctl(fence_fd, SYNC_IOC_MERGE, nullptr);
        close(fence_fd);
        fence_fd = new_fence_fd;
    }
}

int mga::SyncFence::extract_native_handle()
{
    return 8;
}
