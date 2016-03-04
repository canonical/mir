/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_BUFFER_H
#define MIR_CLIENT_BUFFER_H

#include "mir_toolkit/mir_buffer.h"
#include <memory>
#include <chrono>
#include <mutex>

namespace mir
{
namespace protobuf
{
class Buffer;
}
namespace client
{
class ClientBuffer;
class MemoryRegion;
//this is the type backing MirBuffer* 
class Buffer
{
public:
    Buffer(
        mir_buffer_callback cb, void* context,
        int buffer_id,
        std::shared_ptr<ClientBuffer> const& buffer);
    int rpc_id() const;

    void submitted();
    void received(protobuf::Buffer const& update_message);

    MirNativeBuffer* as_mir_native_buffer() const;
    MirGraphicsRegion map_region();

    void set_fence(MirNativeFence*, MirBufferAccess);
    MirNativeFence* get_fence() const;
    bool wait_fence(MirBufferAccess, std::chrono::nanoseconds);

private:
    mir_buffer_callback cb;
    void* cb_context;
    int const buffer_id;
    std::shared_ptr<ClientBuffer> buffer;

    std::mutex mutex;
    bool owned;
    std::shared_ptr<MemoryRegion> mapped_region;
};
}
}
#endif /* MIR_CLIENT_BUFFER_H_ */
