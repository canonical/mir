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
#include "default_client_buffer_stream_factory.h"
#include "buffer_stream.h"

namespace mcl = mir::client;
namespace ml = mir::logging;
namespace mp = mir::protobuf;

mcl::DefaultClientBufferStreamFactory::DefaultClientBufferStreamFactory(std::shared_ptr<mcl::ClientBufferFactory> const& client_buffer_factory,
    std::shared_ptr<mcl::EGLNativeWindowFactory> const& native_window_factory,
    std::shared_ptr<ml::Logger> const& logger)
    : client_buffer_factory(client_buffer_factory),
      native_window_factory(native_window_factory),
      logger(logger)
{
}

std::shared_ptr<mcl::ClientBufferStream> mcl::DefaultClientBufferStreamFactory::make_consumer_stream(mp::DisplayServer& server,
    mp::BufferStream const& protobuf_bs)
{
    return std::make_shared<mcl::BufferStream>(server, mcl::BufferStreamMode::Consumer, client_buffer_factory,
        native_window_factory, protobuf_bs, logger);
}

std::shared_ptr<mcl::ClientBufferStream> mcl::DefaultClientBufferStreamFactory::make_producer_stream(mp::DisplayServer& server,
    mp::BufferStream const& protobuf_bs)
{
    return std::make_shared<mcl::BufferStream>(server, mcl::BufferStreamMode::Producer, client_buffer_factory,
        native_window_factory, protobuf_bs, logger);
}
