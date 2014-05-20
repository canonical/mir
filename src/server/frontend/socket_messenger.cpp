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
#include "mir/frontend/session_credentials.h"
#include "mir/variable_length_array.h"

#include <errno.h>
#include <string.h>

#include <stdexcept>

namespace mf = mir::frontend;
namespace mfd = mf::detail;
namespace bs = boost::system;
namespace ba = boost::asio;

mfd::SocketMessenger::SocketMessenger(std::shared_ptr<ba::local::stream_protocol::socket> const& socket)
    : socket(socket)
{
}

mf::SessionCredentials mfd::SocketMessenger::client_creds()
{
    return mf::SessionCredentials::from_socket_fd(socket->native_handle());
}

void mfd::SocketMessenger::send(char const* data, size_t length, FdSets const& fd_set)
{
    static size_t const header_size{2};
    mir::VariableLengthArray<mf::serialization_buffer_size> whole_message{header_size + length};

    whole_message.data()[0] = static_cast<unsigned char>((length >> 8) & 0xff);
    whole_message.data()[1] = static_cast<unsigned char>((length >> 0) & 0xff);
    std::copy(data, data + length, whole_message.data() + header_size);

    std::unique_lock<std::mutex> lg(message_lock);

    // TODO: This should be asynchronous, but we are not making sure
    // that a potential call to send_fds is executed _after_ this
    // function has completed (if it would be executed asynchronously.
    // NOTE: we rely on this synchronous behavior as per the comment in
    // mf::SessionMediator::create_surface
    ba::write(*socket, ba::buffer(whole_message.data(), whole_message.size()));

    for (auto const& fds : fd_set)
        send_fds_locked(lg, fds);
}

void mfd::SocketMessenger::send_fds_locked(std::unique_lock<std::mutex> const&, std::vector<int32_t> const& fds)
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

        auto sent = sendmsg(socket->native_handle(), &header, 0);
        if (sent < 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to send fds: " + std::string(strerror(errno))));
    }
}

void mfd::SocketMessenger::async_receive_msg(
    MirReadHandler const& handler,
    ba::mutable_buffers_1 const& buffer)
{
    boost::asio::async_read(
         *socket,
         buffer,
         boost::asio::transfer_exactly(ba::buffer_size(buffer)),
         handler);
}

bs::error_code mfd::SocketMessenger::receive_msg(
    ba::mutable_buffers_1 const& buffer)
{
    bs::error_code e;
    boost::asio::read(
         *socket,
         buffer,
         boost::asio::transfer_exactly(ba::buffer_size(buffer)),
         e);
    return e;
}

size_t mfd::SocketMessenger::available_bytes()
{
    boost::asio::socket_base::bytes_readable command{true};
    socket->io_control(command);
    return command.get();
}
