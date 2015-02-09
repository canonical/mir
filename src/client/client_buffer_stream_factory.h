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

#ifndef MIR_CLIENT_CLIENT_BUFFER_STREAM_FACTORY_H_
#define MIR_CLIENT_CLIENT_BUFFER_STREAM_FACTORY_H_

#include "mir_protobuf.pb.h"

#include <memory>

namespace mir
{
namespace client
{
class ClientBufferStream;
class ClientBufferStreamFactory
{
public:
    virtual std::shared_ptr<ClientBufferStream> make_consumer_stream(protobuf::DisplayServer& server,
       protobuf::BufferStream const& protobuf_bs) = 0;
    virtual std::shared_ptr<ClientBufferStream> make_producer_stream(protobuf::DisplayServer& server,
       protobuf::BufferStream const& protobuf_bs) = 0;

protected:
    ClientBufferStreamFactory() = default;
    virtual ~ClientBufferStreamFactory() = default;
    ClientBufferStreamFactory(const ClientBufferStreamFactory&) = delete;
    ClientBufferStreamFactory& operator=(const ClientBufferStreamFactory&) = delete;
};
}
}

#endif // MIR_CLIENT_CLIENT_BUFFER_STREAM_FACTORY_H_
