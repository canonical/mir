/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir_client/mir_client_library.h"
#include "mir_client/android/android_client_buffer.h"

namespace mcl=mir::client;
namespace geom=mir::geometry;

mcl::AndroidClientBuffer::AndroidClientBuffer(std::shared_ptr<AndroidRegistrar> registrar,
                         std::shared_ptr<MirBufferPackage> && package, geom::Size size,
                         geom::PixelFormat pf)
 : creation_package(package),
   buffer_registrar(registrar),
   rect({{geom::X(0),geom::Y(0)}, size}),
   buffer_pf(pf)
{
    creation_package = std::move(package);
    native_handle = std::shared_ptr<const native_handle_t> (convert_to_native_handle(creation_package));

    buffer_registrar->register_buffer(native_handle.get());
}

mcl::AndroidClientBuffer::~AndroidClientBuffer()
{
    buffer_registrar->unregister_buffer(native_handle.get());
}

const native_handle_t* mcl::AndroidClientBuffer::convert_to_native_handle(const std::shared_ptr<MirBufferPackage>& package)
{
    int native_handle_header_size = 3;
    int total = package->fd_items + package->data_items + native_handle_header_size;
    native_handle_t* handle = (native_handle_t*) malloc(sizeof(int) * total );

    handle->version = total;
    handle->numFds  = package->fd_items;
    handle->numInts = package->data_items;

    for(auto i=0; i< handle->numFds; i++)
    {
        handle->data[i] = package->fd[i];
    }

    int offset_i = handle->numFds; 
    for(auto i=0; i< handle->numInts; i++)
    {
        handle->data[offset_i+i] = package->data[i];
    }
 
    return handle;
}

std::shared_ptr<mcl::MemoryRegion> mcl::AndroidClientBuffer::secure_for_cpu_write()
{
    auto vaddr = buffer_registrar->secure_for_cpu(native_handle, rect);
    auto region =  std::make_shared<mcl::MemoryRegion>();
    region->vaddr = vaddr;
    region->width = rect.size.width;
    region->height = rect.size.height;
    region->format = buffer_pf;

    return region;
}

geom::Size mcl::AndroidClientBuffer::size() const
{
    return rect.size;
}

geom::PixelFormat mcl::AndroidClientBuffer::pixel_format() const
{
    return buffer_pf;
}

std::shared_ptr<MirBufferPackage> mcl::AndroidClientBuffer::get_buffer_package() const
{
    return creation_package;
}
