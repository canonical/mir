/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_CLIENT_DEFAULT_CLIENT_BUFFER_STREAM_FACTORY_H_
#define MIR_CLIENT_DEFAULT_CLIENT_BUFFER_STREAM_FACTORY_H_

#include "client_buffer_stream_factory.h"

namespace mir
{
namespace logging
{
class Logger;
}
namespace client
{
class ClientBufferFactory;
class ClientPlatform;

class DefaultClientBufferStreamFactory : public ClientBufferStreamFactory
{
public:
    DefaultClientBufferStreamFactory(
        std::shared_ptr<ClientPlatform> const& native_window_factory,
        std::shared_ptr<logging::Logger> const& logger);
    virtual ~DefaultClientBufferStreamFactory() = default;

    std::shared_ptr<ClientBufferStream> make_consumer_stream(
        MirConnection*, rpc::DisplayServer& server,
        protobuf::BufferStream const& protobuf_bs, std::string const& surface_name) override;
    std::shared_ptr<ClientBufferStream> make_producer_stream(
        MirConnection*, rpc::DisplayServer& server,
        protobuf::BufferStream const& protobuf_bs, std::string const& surface_name) override;

    ClientBufferStream* make_producer_stream(
        MirConnection*,
        rpc::DisplayServer& server,
        protobuf::BufferStreamParameters const& params,
        mir_buffer_stream_callback callback, void* context) override;

private:
    std::shared_ptr<ClientPlatform> const client_platform;
    std::shared_ptr<logging::Logger> const logger;

};
}
}

#endif // MIR_CLIENT_DEFAULT_CLIENT_BUFFER_STREAM_FACTORY_H_
