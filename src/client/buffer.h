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
#include "mir/geometry/size.h"
#include "atomic_callback.h"
#include "mir_buffer.h"
#include <memory>
#include <chrono>
#include <mutex>

namespace mir
{
namespace client
{
class ClientBuffer;
class MemoryRegion;
class Buffer : public MirBuffer
{
public:
    Buffer(
        mir_buffer_callback cb, void* context,
        int buffer_id,
        std::shared_ptr<ClientBuffer> const& buffer,
        MirConnection* connection,
        MirBufferUsage usage);

    int rpc_id() const override;

    void submitted() override;
    void received() override;
    void received(MirBufferPackage const& update_message) override;

    MirNativeBuffer* as_mir_native_buffer() const override;
    std::shared_ptr<ClientBuffer> client_buffer() const override;
    MirGraphicsRegion map_region() override;

    void set_fence(MirNativeFence*, MirBufferAccess) override;
    MirNativeFence* get_fence() const override;
    bool wait_fence(MirBufferAccess, std::chrono::nanoseconds) override;

    MirBufferUsage buffer_usage() const override;
    MirPixelFormat pixel_format() const override;
    geometry::Size size() const override;

    MirConnection* allocating_connection() const override;

    void increment_age() override;
    void set_callback(mir_buffer_callback callback, void* context) override;
private:
    int const buffer_id;
    std::shared_ptr<ClientBuffer> const buffer;

    AtomicCallback<> cb;

    std::mutex mutex;
    bool owned;
    std::shared_ptr<MemoryRegion> mapped_region;
    MirConnection* const connection;
    MirBufferUsage const usage;
};
}
}
#endif /* MIR_CLIENT_BUFFER_H_ */
