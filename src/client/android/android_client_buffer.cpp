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

#include "mir_toolkit/mir_client_library.h"
#include "android_client_buffer.h"

#include <hardware/gralloc.h>
namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace geom=mir::geometry;

mcla::AndroidClientBuffer::AndroidClientBuffer(std::shared_ptr<AndroidRegistrar> const& registrar,
                                               std::shared_ptr<MirBufferPackage> const& package,
                                               geom::Size size, geom::PixelFormat pf)
 : creation_package(package),
   buffer_registrar(registrar),
   rect({{geom::X(0),geom::Y(0)}, size}),
   buffer_pf(pf)
{
    creation_package = std::move(package);
    native_handle = std::shared_ptr<const native_handle_t> (convert_to_native_handle(creation_package));

    buffer_registrar->register_buffer(native_handle.get());

    pack_native_window_buffer();
}

static void incRef(android_native_base_t*)
{
}
void mcla::AndroidClientBuffer::pack_native_window_buffer()
{
    native_window_buffer.height = static_cast<int32_t>(rect.size.height.as_uint32_t());
    native_window_buffer.width =  static_cast<int32_t>(rect.size.width.as_uint32_t());
    native_window_buffer.stride = creation_package->stride;
    native_window_buffer.usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;

    native_window_buffer.handle = native_handle.get();
    native_window_buffer.common.incRef = &incRef;
    native_window_buffer.common.decRef = &incRef;
}

mcla::AndroidClientBuffer::~AndroidClientBuffer()
{
    buffer_registrar->unregister_buffer(native_handle.get());
}

const native_handle_t* mcla::AndroidClientBuffer::convert_to_native_handle(const std::shared_ptr<MirBufferPackage>& package)
{
    int native_handle_header_size = sizeof(native_handle_t);
    int total = package->fd_items + package->data_items + native_handle_header_size;
    native_handle_t* handle = (native_handle_t*) malloc(sizeof(int) * total);

    handle->version = native_handle_header_size;
    handle->numFds  = package->fd_items;
    handle->numInts = package->data_items;

    for (auto i=0; i< handle->numFds; i++)
    {
        handle->data[i] = package->fd[i];
    }

    int offset_i = handle->numFds;
    for (auto i=0; i< handle->numInts; i++)
    {
        handle->data[offset_i+i] = package->data[i];
    }

    return handle;
}

std::shared_ptr<mcl::MemoryRegion> mcla::AndroidClientBuffer::secure_for_cpu_write()
{
    auto vaddr = buffer_registrar->secure_for_cpu(native_handle, rect);
    auto region =  std::make_shared<mcl::MemoryRegion>();
    region->vaddr = vaddr;
    region->width = rect.size.width;
    region->height = rect.size.height;
    region->stride = geom::Stride{creation_package->stride};
    region->format = buffer_pf;

    return region;
}

geom::Size mcla::AndroidClientBuffer::size() const
{
    return rect.size;
}

geom::Stride mcla::AndroidClientBuffer::stride() const
{
    return geom::Stride{creation_package->stride};
}

geom::PixelFormat mcla::AndroidClientBuffer::pixel_format() const
{
    return buffer_pf;
}

MirNativeBuffer mcla::AndroidClientBuffer::get_native_handle()
{
    return &native_window_buffer;
}

std::shared_ptr<MirBufferPackage> mcla::AndroidClientBuffer::get_buffer_package() const
{
    return creation_package;
}
