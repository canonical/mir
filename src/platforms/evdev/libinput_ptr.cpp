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

#include "libinput.h"
#include "libinput_ptr.h"
#include "fd_store.h"

#include <mir/errno_utils.h>

#include <fcntl.h>
#include <iostream>
#include <cstring>

namespace mie = mir::input::evdev;

namespace
{
int fd_open(const char* path, int flags, void* userdata)
{
    auto fd_store = static_cast<mie::FdStore*>(userdata);

    auto fd = fd_store->take_fd(path);

    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        std::cerr << "Failed to set flags: " << mir::errno_to_cstr(errno) << " (" << errno << ")" << std::endl;
    }

    return fd_store->take_fd(path);
}

void fd_close(int fd, void* userdata)
{
    auto fd_store = static_cast<mie::FdStore*>(userdata);

    fd_store->remove_fd(fd);
}

const libinput_interface fd_ops = {fd_open, fd_close};
}

mie::LibInputPtr mie::make_libinput(FdStore* fd_store)
{
    auto ret = mie::LibInputPtr{
        libinput_path_create_context(
            &fd_ops,
            fd_store),
        libinput_unref};
    return ret;
}
