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

#include "fd_store.h"

#include "mir/log.h"

#include <algorithm>
#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mie = mir::input::evdev;

void mie::FdStore::store_fd(char const* path, mir::Fd&& fd)
{
    fds[path] = std::move(fd);
}

mir::Fd mie::FdStore::take_fd(char const* path)
{
    try
    {
        return fds.at(path);
    }
    catch (std::out_of_range const&)
    {
        if (removed.first == path)
        {
            mir::log_warning("Requested fd for path %s was removed", path);
            return fds.insert(std::move(removed)).first->second;
        }
        mir::log_warning("Failed to find requested fd for path %s", path);
    }
    return mir::Fd{};
}

void mie::FdStore::remove_fd(int fd)
{
    auto element = std::find_if(
        fds.begin(),
        fds.end(),
        [fd](auto const& pair)
        {
            return pair.second == fd;
        });

    if (element == fds.end())
    {
        mir::log_warning("Attempted to remove unmanaged fd %i", fd);
    }
    else
    {
        // We keep the last removed element in case libinput asks for it
        removed = *element;
        fds.erase(element);
    }
}
