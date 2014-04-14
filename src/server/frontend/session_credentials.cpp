/*
 * Copyright © 2014 Canonical Ltd.
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
 */

#include "mir/frontend/session_credentials.h"

#include <sys/socket.h>

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;

mf::SessionCredentials::SessionCredentials(int socket_fd)
{
    struct ucred cr;
    socklen_t cl = sizeof(cr);

    auto status = getsockopt(socket_fd, SOL_SOCKET, SO_PEERCRED, &cr, &cl);

    if (status)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to query client socket credentials"));

    the_pid = cr.pid;
    the_uid = cr.uid;
    the_gid = cr.gid;
}

pid_t mf::SessionCredentials::pid() const
{
    return the_pid;
}

uid_t mf::SessionCredentials::uid() const
{
    return the_uid;
}

gid_t mf::SessionCredentials::gid() const
{
    return the_gid;
}
