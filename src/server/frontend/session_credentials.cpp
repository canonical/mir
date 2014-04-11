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

mf::SessionCredentials::SessionCredentials(pid_t pid, uid_t uid, gid_t gid)
    : pid_(pid), uid_(uid), gid_(gid)
{}

pid_t mf::SessionCredentials::pid() const
{
    return pid_;
}

uid_t mf::SessionCredentials::uid() const
{
    return uid_;
}

gid_t mf::SessionCredentials::gid() const
{
    return gid_;
}

mf::SessionCredentials mf::SessionCredentials::from_current_process()
{
    pid_t the_pid = ::getpid();
    uid_t the_uid = ::geteuid();
    gid_t the_gid = ::getegid();
    return mf::SessionCredentials{the_pid, the_uid, the_gid};
}

mf::SessionCredentials mf::SessionCredentials::from_socket(int fd)
{
    struct ucred cr;
    socklen_t cl = sizeof(cr);

    auto status = getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cr, &cl);

    if (status)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to query client socket credentials"));

    return mf::SessionCredentials{cr.pid, cr.uid, cr.gid};
}
