/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/client_buffer.h"
#include "buffer.h"
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;

mcl::Buffer::Buffer(
    mir_buffer_callback cb, void* context,
    int buffer_id,
    std::shared_ptr<ClientBuffer> const& buffer,
    MirConnection* connection) :
    cb(cb),
    cb_context(context),
    buffer_id(buffer_id),
    buffer(buffer),
    owned(true),
    connection(connection)
{
    cb(nullptr, reinterpret_cast<MirBuffer*>(this), cb_context);
}

int mcl::Buffer::rpc_id() const
{
    return buffer_id;
}

void mcl::Buffer::submitted()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    if (!owned)
        BOOST_THROW_EXCEPTION(std::logic_error("cannot submit unowned buffer"));
    mapped_region.reset();
    owned = false;
}

void mcl::Buffer::received(MirBufferPackage const& update_package)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    if (!owned)
    {
        owned = true;
        buffer->update_from(update_package);
        cb(nullptr, reinterpret_cast<MirBuffer*>(this), cb_context);
    }
}
    
MirGraphicsRegion mcl::Buffer::map_region()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    mapped_region = buffer->secure_for_cpu_write();
    return MirGraphicsRegion {
        mapped_region->width.as_int(),
        mapped_region->height.as_int(),
        mapped_region->stride.as_int(),
        mapped_region->format,
        mapped_region->vaddr.get()
    };
}

MirNativeBuffer* mcl::Buffer::as_mir_native_buffer() const
{
    return buffer->as_mir_native_buffer();
}

void mcl::Buffer::set_fence(MirNativeFence* native_fence, MirBufferAccess access)
{
    buffer->set_fence(native_fence, access);
}

MirNativeFence* mcl::Buffer::get_fence() const
{
    return buffer->get_fence();
}

bool mcl::Buffer::wait_fence(MirBufferAccess access, std::chrono::nanoseconds timeout)
{
    return buffer->wait_fence(access, timeout);
}

MirConnection* mcl::Buffer::allocating_connection() const
{
    return connection;
}
