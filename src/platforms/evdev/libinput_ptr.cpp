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
#include <sys/stat.h>

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

    // libinput will call fd_close on the fd if it receives a tablet switch event
    // or a lid switch event, which causes the fd to be removed from the store.
    // However, Mir does not actually open fds whenever libinput requests, instead they
    // are opened whenever a device is added, and closed whenever it's removed. Therefore we
    // can check if the path for the fd still exists in devtmpfs, and if it does, we can assume
    // that the fd is still valid and should not be removed from the store.
    try
    {
        auto path = fd_store->path_for(fd);

        struct stat sb;
        if (::stat(path.c_str(), &sb) == 0)
            return;

        if (errno == ENOENT)
        {
            fd_store->remove_fd(fd);
        }
    } catch (...) {
        // Seems like we got called for an FD that we don't track?
        return;
    }
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
