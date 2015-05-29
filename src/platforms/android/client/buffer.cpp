/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/android_native_buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir_toolkit/mir_client_library.h"
#include "buffer_registrar.h"
#include "buffer.h"
#include <hardware/gralloc.h>

namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace geom=mir::geometry;
namespace mga=mir::graphics::android;

mcla::Buffer::Buffer(
    std::shared_ptr<BufferRegistrar> const& registrar,
    MirBufferPackage const& package,
    MirPixelFormat pf) :
    buffer_registrar{registrar},
    native_buffer{registrar->register_buffer(package, pf)},
    buffer_pf(pf),
    buffer_stride{package.stride},
    buffer_size{package.width, package.height}
{
}

std::shared_ptr<mcl::MemoryRegion> mcla::Buffer::secure_for_cpu_write()
{
    auto rect = geom::Rectangle{geom::Point{0, 0}, size()};
    auto vaddr = buffer_registrar->secure_for_cpu(native_buffer, rect);
    auto region =  std::make_shared<mcl::MemoryRegion>();
    region->vaddr = vaddr;
    region->width = rect.size.width;
    region->height = rect.size.height;
    region->stride = stride();
    region->format = buffer_pf;
    return region;
}

geom::Size mcla::Buffer::size() const
{
    return buffer_size;
}

geom::Stride mcla::Buffer::stride() const
{
    return buffer_stride;
}

MirPixelFormat mcla::Buffer::pixel_format() const
{
    return buffer_pf;
}

std::shared_ptr<mir::graphics::NativeBuffer> mcla::Buffer::native_buffer_handle() const
{
    return native_buffer;
}

void mcla::Buffer::update_from(MirBufferPackage const& update_package)
{
    if ((update_package.data_items != 0) && 
        (update_package.fd_items != 0) && 
        (update_package.data[0] == static_cast<int>(mga::BufferFlag::fenced)))
    {
        auto fence_fd = update_package.fd[0];
        native_buffer->update_usage(fence_fd, mga::BufferAccess::read);
    }
}

void mcla::Buffer::fill_update_msg(MirBufferPackage& message)
{
    message.data_items = 1;
    auto fence = native_buffer->copy_fence();
    if (fence > 0)
    {
        message.data[0] = static_cast<int>(mga::BufferFlag::fenced);
        message.fd[0] = fence;
        message.fd_items = 1; 
    }
    else
    {
        message.data[0] = static_cast<int>(mga::BufferFlag::unfenced);
        message.fd_items = 0; 
    }
}
