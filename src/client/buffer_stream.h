/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_CLIENT_BUFFER_STREAM_H
#define MIR_CLIENT_BUFFER_STREAM_H

#include "mir_wait_handle.h"

#include "mir_protobuf.pb.h"

#include <memory>

namespace mir
{
namespace client
{
class ClientBufferFactory;

enum BufferStreamMode
{
Producer, // Like surfaces
Consumer // Like screencasts
};

class BufferStream;
typedef void (*mir_buffer_stream_callback)(
    BufferStream*, void*);

class BufferStream
{
public:
    BufferStream(mir::protobuf::DisplayServer const& server,
        BufferStreamMode mode,
        std::shared_ptr<ClientBufferFactory> const& buffer_factory,
                 protobuf::BufferStream const& protobuf_bs);
    virtual ~BufferStream() = default;
    
    MirWaitHandle* next_buffer(mir_buffer_stream_callback callback, void* context);

protected:
    BufferStream(BufferStream const&) = delete;
    BufferStream& operator=(BufferStream const&) = delete;

private:
    mir::protobuf::DisplayServer &display_server;

    BufferStreamMode const mode;
    std::shared_ptr<ClientBufferFactory> const buffer_factory;
};

}
}

#endif // MIR_CLIENT_BUFFER_STREAM_H
