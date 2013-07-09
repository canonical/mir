/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "socket_session.h"

#include <boost/signals2.hpp>
#include <sys/types.h>
#include <sys/socket.h>

namespace ba = boost::asio;
namespace bs = boost::system;

namespace mfd = mir::frontend::detail;

mfd::SocketSession::SocketSession(
    boost::asio::io_service& io_service,
    int id_,
    std::shared_ptr<ConnectedSessions<SocketSession>> const& connected_sessions) :
    socket(io_service),
    id_(id_),
    connected_sessions(connected_sessions),
    processor(std::make_shared<NullMessageProcessor>())
{
}

mfd::SocketSession::~SocketSession() noexcept
{
}

void mfd::SocketSession::send(std::string const& body)
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
    ba::write(socket, ba::buffer(whole_message));
}

void mfd::SocketSession::send_fds(std::vector<int32_t> const& fds)
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

        sendmsg(socket.native_handle(), &header, 0);
    }
}

void mfd::SocketSession::read_next_message()
{
    ba::async_read(socket,
        ba::buffer(message_header_bytes),
        boost::bind(&mfd::SocketSession::on_read_size,
                    this, ba::placeholders::error));
}

void mfd::SocketSession::on_read_size(const boost::system::error_code& error)
{
    if (error)
    {
        connected_sessions->remove(id());
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }

    size_t const body_size = (message_header_bytes[0] << 8) + message_header_bytes[1];

    ba::async_read(
         socket,
         message,
         ba::transfer_exactly(body_size),
         boost::bind(&mfd::SocketSession::on_new_message,
                     this, ba::placeholders::error));
}

void mfd::SocketSession::on_new_message(const boost::system::error_code& error)
{
    if (error)
    {
        connected_sessions->remove(id());
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }

    std::istream msg(&message);

    if (processor->process_message(msg))
    {
        read_next_message();
    }
    else
    {
        connected_sessions->remove(id());
    }
}

void mfd::SocketSession::on_response_sent(bs::error_code const& error, std::size_t)
{
    if (error)
    {
        connected_sessions->remove(id());
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }
}
