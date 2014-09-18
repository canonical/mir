/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/fd_socket_transmission.h"
#include "mir/variable_length_array.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>

void mir::send_fds(
    mir::Fd const& socket,
    std::vector<mir::Fd> const& fds)
{
    if (fds.size() > 0)
    {
        // We send dummy data
        struct iovec iov;
        char dummy_iov_data = 'M';
        iov.iov_base = &dummy_iov_data;
        iov.iov_len = 1;

        // Allocate space for control message
        static auto const builtin_n_fds = 5;
        static auto const builtin_cmsg_space = CMSG_SPACE(builtin_n_fds * sizeof(int));
        auto const fds_bytes = fds.size() * sizeof(int);
        mir::VariableLengthArray<builtin_cmsg_space> control{CMSG_SPACE(fds_bytes)};
        // Silence valgrind uninitialized memory complaint
        memset(control.data(), 0, control.size());

        // Message to send
        struct msghdr header;
        header.msg_name = NULL;
        header.msg_namelen = 0;
        header.msg_iov = &iov;
        header.msg_iovlen = 1;
        header.msg_controllen = control.size();
        header.msg_control = control.data();
        header.msg_flags = 0;

        // Control message contains file descriptors
        struct cmsghdr *message = CMSG_FIRSTHDR(&header);
        message->cmsg_len = CMSG_LEN(fds_bytes);
        message->cmsg_level = SOL_SOCKET;
        message->cmsg_type = SCM_RIGHTS;

        int* const data = reinterpret_cast<int*>(CMSG_DATA(message));
        int i = 0;
        for (auto& fd : fds)
            data[i++] = fd;

        auto const sent = sendmsg(socket, &header, 0);
        if (sent < 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to send fds: " + std::string(strerror(errno))));
    }
}
