/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#include "mir/client/client_buffer.h"
#include "buffer.h"
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
mcl::Buffer::Buffer(
    MirBufferCallback cb, void* context,
    int buffer_id,
    std::shared_ptr<ClientBuffer> const& buffer,
    MirConnection* connection,
    MirBufferUsage usage) :
    buffer_id(buffer_id),
    buffer(buffer),
    cb([this, cb, context]{ (*cb)(reinterpret_cast<::MirBuffer*>(this), context); }),
    owned(false),
    connection(connection),
    usage(usage)
{
}
#pragma GCC diagnostic pop

int mcl::Buffer::rpc_id() const
{
    return buffer_id;
}

void mcl::Buffer::submitted()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    if (!owned)
        BOOST_THROW_EXCEPTION(std::logic_error("cannot submit unowned buffer"));
    buffer->mark_as_submitted();
    mapped_region.reset();
    owned = false;
}

void mcl::Buffer::received()
{
    {
        std::lock_guard<decltype(mutex)> lk(mutex);
        if (!owned)
            owned = true;
    }

    cb();
}

void mcl::Buffer::received(MirBufferPackage const& update_package)
{
    {
        std::lock_guard<decltype(mutex)> lk(mutex);
        if (!owned)
        {
            owned = true;
            buffer->update_from(update_package);
        }
    }

    cb();
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

void mcl::Buffer::unmap_region()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    mapped_region = nullptr;
}

MirConnection* mcl::Buffer::allocating_connection() const
{
    return connection;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirBufferUsage mcl::Buffer::buffer_usage() const
{
#pragma GCC diagnostic pop
    return usage;
}

MirPixelFormat mcl::Buffer::pixel_format() const
{
    return buffer->pixel_format();
}

mir::geometry::Size mcl::Buffer::size() const
{
    return buffer->size();
}
std::shared_ptr<mcl::ClientBuffer> mcl::Buffer::client_buffer() const
{
    return buffer;
}

void mcl::Buffer::increment_age()
{
    buffer->increment_age();
}

bool mcl::Buffer::valid() const
{
    return true;
}

char const* mcl::Buffer::error_message() const
{
    return "";
}

void mcl::Buffer::set_callback(MirBufferCallback callback, void* context)
{
    cb.set_callback([&, callback, context]{ (*callback)(reinterpret_cast<::MirBuffer*>(this), context); });
}
