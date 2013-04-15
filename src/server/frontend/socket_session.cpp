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

#include "ancillary.h"

#include <boost/signals2.hpp>

namespace ba = boost::asio;
namespace bs = boost::system;

namespace mfd = mir::frontend::detail;


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

void mfd::SocketSession::send_fds(std::vector<int32_t> const& fd)
{
    if (fd.size() > 0)
    {
        ancil_send_fds(socket.native_handle(), fd.data(), fd.size());
    }
}

void mfd::SocketSession::read_next_message()
{
    ba::async_read(socket,
        ba::buffer(message_header_bytes),
        boost::bind(&mfd::SocketSession::on_read_size,
                    this, ba::placeholders::error));
}

void mfd::SocketSession::on_read_size(const boost::system::error_code& ec)
{
    if (!ec)
    {
        size_t const body_size = (message_header_bytes[0] << 8) + message_header_bytes[1];
        // Read newline delimited messages for now
        ba::async_read(
             socket,
             message,
             ba::transfer_exactly(body_size),
             boost::bind(&mfd::SocketSession::on_new_message,
                         this, ba::placeholders::error));
    }
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
