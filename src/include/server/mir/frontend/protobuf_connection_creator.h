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

#ifndef MIR_FRONTEND_PROTOBUF_CONNECTION_CREATOR_H_
#define MIR_FRONTEND_PROTOBUF_CONNECTION_CREATOR_H_

#include "mir/frontend/connection_creator.h"
#include "mir/frontend/connections.h"

#include <atomic>

namespace mir
{
namespace graphics { class PlatformIpcOperations; }
namespace frontend
{
class MessageProcessorReport;
class ProtobufIpcFactory;
class SessionAuthorizer;

namespace detail
{
class DisplayServer;
class SocketConnection;
class MessageProcessor;
class ProtobufMessageSender;
}

class ProtobufConnectionCreator : public ConnectionCreator
{
public:
    ProtobufConnectionCreator(
        std::shared_ptr<ProtobufIpcFactory> const& ipc_factory,
        std::shared_ptr<SessionAuthorizer> const& session_authorizer,
        std::shared_ptr<graphics::PlatformIpcOperations> const& operations,
        std::shared_ptr<MessageProcessorReport> const& report);
    ~ProtobufConnectionCreator() noexcept;

    void create_connection_for(
        std::shared_ptr<boost::asio::local::stream_protocol::socket> const& socket,
        ConnectionContext const& connection_context) override;

    virtual std::shared_ptr<detail::MessageProcessor> create_processor(
        std::shared_ptr<detail::ProtobufMessageSender> const& sender,
        std::shared_ptr<detail::DisplayServer> const& display_server,
        std::shared_ptr<MessageProcessorReport> const& report) const;

private:
    int next_id();

    std::shared_ptr<ProtobufIpcFactory> const ipc_factory;
    std::shared_ptr<SessionAuthorizer> const session_authorizer;
    std::shared_ptr<graphics::PlatformIpcOperations> const operations;
    std::shared_ptr<MessageProcessorReport> const report;
    std::atomic<int> next_session_id;
    std::shared_ptr<detail::Connections<detail::SocketConnection>> const connections;
};
}
}

#endif /* MIR_FRONTEND_PROTOBUF_CONNECTION_CREATOR_H_ */
