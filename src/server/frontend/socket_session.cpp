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
#include "message_sender.h"
#include "message_receiver.h"
#include "mir/frontend/message_processor.h"

#include "mir_protobuf_wire.pb.h"

#include <boost/signals2.hpp>
#include <boost/throw_exception.hpp>

#include <stdexcept>

#include <sys/types.h>
#include <sys/socket.h>

namespace ba = boost::asio;
namespace bs = boost::system;

namespace mfd = mir::frontend::detail;

mfd::SocketSession::SocketSession(
    std::shared_ptr<mfd::MessageReceiver> const& receiver,
    int id_,
    std::shared_ptr<ConnectedSessions<SocketSession>> const& connected_sessions,
    std::shared_ptr<MessageProcessor> const& processor)
     : socket_receiver(receiver),
       id_(id_),
       connected_sessions(connected_sessions),
       processor(processor)
{
}

mfd::SocketSession::~SocketSession() noexcept
{
}


void mfd::SocketSession::read_next_message()
{
    auto callback = std::bind(&mfd::SocketSession::on_read_size,
                        this, std::placeholders::_1);
    socket_receiver->async_receive_msg(callback, ba::buffer(header, header_size));
}

void mfd::SocketSession::on_read_size(const boost::system::error_code& error)
{
    if (error)
    {
        connected_sessions->remove(id());
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }

    unsigned char const high_byte = header[0];
    unsigned char const low_byte = header[1];
    size_t const body_size = (high_byte << 8) + low_byte;

    body.resize(body_size);

    if (socket_receiver->available_bytes() >= body_size)
    {
        on_new_message(socket_receiver->receive_msg(ba::buffer(body)));
    }
    else
    {
        auto callback = std::bind(&mfd::SocketSession::on_new_message,
                                  this, std::placeholders::_1);
        socket_receiver->async_receive_msg(callback, ba::buffer(body));
    }
}

void mfd::SocketSession::on_new_message(const boost::system::error_code& error)
try
{
    if (error)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }

    mir::protobuf::wire::Invocation invocation;
    invocation.ParseFromArray(body.data(), body.size());

    if (!invocation.has_protocol_version() || invocation.protocol_version() != 1)
        BOOST_THROW_EXCEPTION(std::runtime_error("Unsupported protocol version"));

    if (processor->dispatch(invocation))
    {
        read_next_message();
    }
    else
    {
        connected_sessions->remove(id());
    }
}
catch (...)
{
    connected_sessions->remove(id());
    throw;
}

void mfd::SocketSession::on_response_sent(bs::error_code const& error, std::size_t)
{
    if (error)
    {
        connected_sessions->remove(id());
        BOOST_THROW_EXCEPTION(std::runtime_error(error.message()));
    }
}
