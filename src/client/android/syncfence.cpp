/*
 * Copyright © 2012 Canonical Ltd.
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

namespace
{
class IoctlControl : public mcla::IoctlWrapper
{
public:
    int ioctl(int fd, unsigned long int request, int timeout)
    {
        return ioctl(fd, request, timeout);
    }

    int close(int fd)
    {
        return close(fd);
    }
};
}

mcla::SyncFence::SyncFence(int /*fd*/)
 : ioctl_wrapper(std::make_shared<IoctlControl>())
{
}

mcla::SyncFence::SyncFence(int /*fd*/, std::shared_ptr<IoctlWrapper> const& wrapper)
 : ioctl_wrapper(wrapper)
{
}

void mcla::SyncFence::wait()
{

}
