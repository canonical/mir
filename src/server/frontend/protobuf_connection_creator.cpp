/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include "mir/frontend/protobuf_connection_creator.h"

#include "mir/frontend/session_credentials.h"
#include "event_sender.h"
#include "protobuf_message_processor.h"
#include "protobuf_responder.h"
#include "socket_messenger.h"
#include "socket_connection.h"

#include "protobuf_ipc_factory.h"
#include "mir/frontend/session_authorizer.h"

namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;
namespace ba = boost::asio;

mf::ProtobufConnectionCreator::ProtobufConnectionCreator(
    std::shared_ptr<ProtobufIpcFactory> const& ipc_factory,
    std::shared_ptr<SessionAuthorizer> const& session_authorizer,
    std::shared_ptr<MessageProcessorReport> const& report)
:   ipc_factory(ipc_factory),
    session_authorizer(session_authorizer),
    report(report),
    next_session_id(0),
    connections(std::make_shared<mfd::Connections<mfd::SocketConnection>>())
{
}

mf::ProtobufConnectionCreator::~ProtobufConnectionCreator() noexcept
{
    connections->clear();
}

int mf::ProtobufConnectionCreator::next_id()
{
    return next_session_id.fetch_add(1);
}

void mf::ProtobufConnectionCreator::create_connection_for(
    std::shared_ptr<boost::asio::local::stream_protocol::socket> const& socket,
    ConnectionContext const& connection_context)
{
    auto const messenger = std::make_shared<detail::SocketMessenger>(socket);
    auto const creds = messenger->client_creds();

    if (session_authorizer->connection_is_allowed(creds))
    {
        auto const message_sender = std::make_shared<detail::ProtobufResponder>(
            messenger,
            ipc_factory->resource_cache());

        auto const event_sink = std::make_shared<detail::EventSender>(messenger);
        auto const msg_processor = create_processor(
            message_sender,
            ipc_factory->make_ipc_server(creds, event_sink, connection_context),
            report);

        auto const& connection = std::make_shared<mfd::SocketConnection>(messenger, next_id(), connections, msg_processor);
        connections->add(connection);
        connection->read_next_message();
    }
}

std::shared_ptr<mfd::MessageProcessor>
mf::ProtobufConnectionCreator::create_processor(
    std::shared_ptr<mfd::ProtobufMessageSender> const& sender,
    std::shared_ptr<detail::DisplayServer> const& display_server,
    std::shared_ptr<mf::MessageProcessorReport> const& report) const
{
    return std::make_shared<detail::ProtobufMessageProcessor>(
        sender,
        display_server,
        report);
}
