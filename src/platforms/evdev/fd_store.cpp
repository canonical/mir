/*
 * Copyright © 2017 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "fd_store.h"

namespace mie = mir::input::evdev;

void mie::FdStore::store_fd(char const* path, mir::Fd&& fd)
{
    fds.insert(std::make_pair(path, std::move(fd)));
}

mir::Fd&& mie::FdStore::take_fd(char const* path)
{
    auto fd = std::move(fds.at(path));
    fds.erase(path);

    return std::move(fd);
}
