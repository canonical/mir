/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#ifndef MIR_FRONTEND_PROTOBUF_IPC_FACTORY_H_
#define MIR_FRONTEND_PROTOBUF_IPC_FACTORY_H_

#include <sys/types.h>

#include <memory>

namespace mir
{
namespace frontend
{
namespace detail
{
class DisplayServer;
}
class ConnectionContext;
class EventSink;
class ResourceCache;
class SessionCredentials;
class MessageSender;

typedef std::function<std::unique_ptr<EventSink>(std::shared_ptr<MessageSender>)> EventSinkFactory;

class ProtobufIpcFactory
{
public:
    virtual std::shared_ptr<detail::DisplayServer> make_ipc_server(
        SessionCredentials const &creds,
        EventSinkFactory const& sink_factory,
        std::shared_ptr<MessageSender> const& message_sender,
        ConnectionContext const &connection_context) = 0;

    virtual std::shared_ptr<ResourceCache> resource_cache() = 0;

protected:
    ProtobufIpcFactory() {}
    virtual ~ProtobufIpcFactory() {}
    ProtobufIpcFactory(ProtobufIpcFactory const&) = delete;
    ProtobufIpcFactory& operator =(ProtobufIpcFactory const&) = delete;
};

}
}

#endif // MIR_FRONTEND_PROTOBUF_IPC_FACTORY_H_
