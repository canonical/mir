/*
 * Copyright © 2013,2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <system_error>
#include <sys/types.h>
#include <sys/socket.h>

#include "channel.h"

#include <boost/throw_exception.hpp>

namespace mi = mir::input;

namespace
{
// Default is 128KB which is larger then we need. Use a smaller size.
int const buffer_size{32 * 1024};
}

mi::Channel::Channel()
{
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sockets)) {
        BOOST_THROW_EXCEPTION(std::system_error(
            errno,
            std::system_category(),
            "Could not create socket pair."));
    }
    server_fd_ = mir::Fd(sockets[0]);
    client_fd_ = mir::Fd(sockets[1]);

    int size = buffer_size;
    setsockopt(server_fd_, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    setsockopt(server_fd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    setsockopt(client_fd_, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    setsockopt(client_fd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

int mi::Channel::server_fd() const
{
    return server_fd_;
}

int mi::Channel::client_fd() const
{
    return client_fd_;
}
