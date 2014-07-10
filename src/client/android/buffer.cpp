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
    std::shared_ptr<const native_handle_t> const& handle,
    geom::Size size,
    MirPixelFormat pf,
    geometry::Stride stride) :
    buffer_registrar(registrar),
    native_handle(handle),
    buffer_pf(pf),
    buffer_stride{stride},
    buffer_size(size)
{
    auto ops = std::make_shared<mga::RealSyncFileOps>();
    auto fence = std::make_shared<mga::SyncFence>(ops, -1);
    auto anwb = std::shared_ptr<mga::RefCountedNativeBuffer>(
        new mga::RefCountedNativeBuffer(handle),
        [](mga::RefCountedNativeBuffer* buffer)
        {
            buffer->mir_dereference();
        });

    anwb->height = static_cast<int32_t>(buffer_size.height.as_uint32_t());
    anwb->width =  static_cast<int32_t>(buffer_size.width.as_uint32_t());
    //note: mir uses stride in bytes, ANativeWindowBuffer needs it in pixel units. some drivers care
    //about byte-stride, they will pass stride via ANativeWindowBuffer::handle (which is opaque to us)
    anwb->stride = stride.as_uint32_t() /
                                   MIR_BYTES_PER_PIXEL(buffer_pf);
    anwb->usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    anwb->handle = native_handle.get();

    native_window_buffer = std::make_shared<mga::AndroidNativeBuffer>(anwb, fence, mga::BufferAccess::read);
}

mcla::Buffer::~Buffer() noexcept
{
}

std::shared_ptr<mcl::MemoryRegion> mcla::Buffer::secure_for_cpu_write()
{
    auto rect = geom::Rectangle{geom::Point{0, 0}, size()};
    auto vaddr = buffer_registrar->secure_for_cpu(native_handle, rect);
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
    return native_window_buffer;
}

void mcla::Buffer::update_from(MirBufferPackage const& update_package)
{
    (void) update_package;
}
