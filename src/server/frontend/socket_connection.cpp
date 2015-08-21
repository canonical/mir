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

#include "socket_connection.h"
#include "message_sender.h"
#include "message_receiver.h"
#include "mir/frontend/message_processor.h"
#include "mir/frontend/session_credentials.h"
#include "mir/protobuf/protocol_version.h"

#include "mir_protobuf_wire.pb.h"

#include <boost/signals2.hpp>
#include <boost/throw_exception.hpp>

#include <stdexcept>

#include <sys/types.h>
#include <sys/socket.h>

namespace ba = boost::asio;
namespace bs = boost::system;

namespace mfd = mir::frontend::detail;

mfd::SocketConnection::SocketConnection(
    std::shared_ptr<mfd::MessageReceiver> const& message_receiver,
    int id_,
    std::shared_ptr<Connections<SocketConnection>> const& connections,
    std::shared_ptr<MessageProcessor> const& processor)
     : message_receiver(message_receiver),
       id_(id_),
       connections(connections),
       processor(processor)
{
}

mfd::SocketConnection::~SocketConnection() noexcept
{
}


void mfd::SocketConnection::read_next_message()
{
    auto callback = std::bind(&mfd::SocketConnection::on_read_size,
                        this, std::placeholders::_1);
    message_receiver->async_receive_msg(callback, ba::buffer(header, header_size));
}

void mfd::SocketConnection::on_read_size(const boost::system::error_code& error)
{
    if (error)
    {
        connections->remove(id());
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }

    unsigned char const high_byte = header[0];
    unsigned char const low_byte = header[1];
    size_t const body_size = (high_byte << 8) + low_byte;

    body.resize(body_size);

    if (message_receiver->available_bytes() >= body_size)
    {
        on_new_message(message_receiver->receive_msg(ba::buffer(body)));
    }
    else
    {
        auto callback = std::bind(&mfd::SocketConnection::on_new_message,
                                  this, std::placeholders::_1);
        message_receiver->async_receive_msg(callback, ba::buffer(body));
    }
}

void mfd::SocketConnection::on_new_message(const boost::system::error_code& error)
try
{
    // No support for newer client libraries
    static auto const forward_compatibility_limit = mir::protobuf::current_protocol_version();
    static auto const backward_compatibility_limit = mir::protobuf::oldest_compatible_protocol_version();

    if (error)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }

    mir::protobuf::wire::Invocation invocation;
    invocation.ParseFromArray(body.data(), body.size());

    if (!invocation.has_protocol_version() ||
        invocation.protocol_version() > forward_compatibility_limit ||
        invocation.protocol_version() < backward_compatibility_limit)
        BOOST_THROW_EXCEPTION(std::runtime_error("Unsupported protocol version"));

    std::vector<mir::Fd> fds;
    if (invocation.side_channel_fds() > 0)
    {
        fds.resize(invocation.side_channel_fds());
        message_receiver->receive_fds(fds);
    }

    if (!client_pid)
    {
        client_pid = message_receiver->client_creds().pid();
        processor->client_pid(client_pid);
    }

    if (processor->dispatch(invocation, fds))
    {
        read_next_message();
    }
    else
    {
        connections->remove(id());
    }
}
catch (...)
{
    connections->remove(id());
    throw;
}

void mfd::SocketConnection::on_response_sent(bs::error_code const& error, std::size_t)
{
    if (error)
    {
        connections->remove(id());
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }
}
