/*
 * Copyright © 2013 Canonical Ltd.
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

#include "socket_messenger.h"
#include "mir/frontend/client_constants.h"

namespace mfd = mir::frontend::detail;
namespace bs = boost::system;
namespace ba = boost::asio;

mfd::SocketMessenger::SocketMessenger(std::shared_ptr<ba::local::stream_protocol::socket> const& socket)
    : socket(socket)
{
    whole_message.reserve(serialization_buffer_size);
}

pid_t mfd::SocketMessenger::client_pid()
{
    struct ucred cr;
    socklen_t cl = sizeof(cr);
    
    auto status = getsockopt(socket->native_handle(), SOL_SOCKET, SO_PEERCRED, &cr, &cl);
    
    if (status)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to query client socket credentials"));
    return cr.pid;
}

void mfd::SocketMessenger::send(std::string const& body)
{
    const size_t size = body.size();
    const unsigned char header_bytes[2] =
    {
        static_cast<unsigned char>((size >> 8) & 0xff),
        static_cast<unsigned char>((size >> 0) & 0xff)
    };

    whole_message.resize(sizeof header_bytes + size);
    std::copy(header_bytes, header_bytes + sizeof header_bytes, whole_message.begin());
    std::copy(body.begin(), body.end(), whole_message.begin() + sizeof header_bytes);

    // TODO: This should be asynchronous, but we are not making sure
    // that a potential call to send_fds is executed _after_ this
    // function has completed (if it would be executed asynchronously.
    // NOTE: we rely on this synchronous behavior as per the comment in
    // mf::SessionMediator::create_surface
    boost::system::error_code err;
    ba::write(*socket, ba::buffer(whole_message), err);
}

void mfd::SocketMessenger::send_fds(std::vector<int32_t> const& fds)
{
    auto n_fds = fds.size();
    if (n_fds > 0)
    {
        // We send dummy data
        struct iovec iov;
        char dummy_iov_data = 'M';
        iov.iov_base = &dummy_iov_data;
        iov.iov_len = 1;

        // Allocate space for control message
        std::vector<char> control(sizeof(struct cmsghdr) + sizeof(int) * n_fds);

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
        message->cmsg_len = header.msg_controllen;
        message->cmsg_level = SOL_SOCKET;
        message->cmsg_type = SCM_RIGHTS;
        int *data = (int *)CMSG_DATA(message);
        int i = 0;
        for (auto &fd: fds)
            data[i++] = fd;

        sendmsg(socket->native_handle(), &header, 0);
    }
}

void mfd::SocketMessenger::async_receive_msg(
    MirReadHandler const& handler, ba::streambuf& buffer, size_t size)
{
    boost::asio::async_read(
         *socket,
         buffer,
         boost::asio::transfer_exactly(size),
         handler);
} 
