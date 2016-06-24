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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "client_buffer.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <system_error>

#include <errno.h>
#include <sys/mman.h>

namespace mcl=mir::client;
namespace mcle=mir::client::eglstream;
namespace geom=mir::geometry;

namespace
{

struct ShmMemoryRegion : mcl::MemoryRegion
{
    ShmMemoryRegion(
        int buffer_fd,
        geom::Size const& size_param,
        geom::Stride stride_param,
        MirPixelFormat format_param)
        : size_in_bytes{size_param.height.as_uint32_t() * stride_param.as_uint32_t()}
    {
        static off_t const map_offset = 0;
        width = size_param.width;
        height = size_param.height;
        stride = stride_param;
        format = format_param;

        void* map = mmap(
            nullptr,
            size_in_bytes,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            buffer_fd,
            map_offset);
        if (map == MAP_FAILED)
        {
            BOOST_THROW_EXCEPTION((
                std::system_error{errno, std::system_category(), "Failed to mmap buffer"}));
        }

        vaddr = std::shared_ptr<char>(static_cast<char*>(map), [](auto*) noexcept {});
    }

    ~ShmMemoryRegion()
    {
        munmap(vaddr.get(), size_in_bytes);
    }

    size_t const size_in_bytes;
};

}

mcle::ClientBuffer::ClientBuffer(
    std::shared_ptr<MirBufferPackage> const& package,
    geom::Size size, MirPixelFormat pf)
    : creation_package{package},
      rect({geom::Point{0, 0}, size}),
      buffer_pf{pf}
{
    if (package->fd_items != 1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Buffer package does not contain the expected number of fd items"));
    }
}

mcle::ClientBuffer::~ClientBuffer() noexcept
{
    // TODO (@raof): Error reporting? It should not be possible for this to fail; if it does,
    //               something's seriously wrong.
    close(creation_package->fd[0]);
}

std::shared_ptr<mcl::MemoryRegion> mcle::ClientBuffer::secure_for_cpu_write()
{
    int const buffer_fd = creation_package->fd[0];

    return std::make_shared<ShmMemoryRegion>(
        buffer_fd,
        size(),
        stride(),
        pixel_format());
}

geom::Size mcle::ClientBuffer::size() const
{
    return rect.size;
}

geom::Stride mcle::ClientBuffer::stride() const
{
    return geom::Stride{creation_package->stride};
}

MirPixelFormat mcle::ClientBuffer::pixel_format() const
{
    return buffer_pf;
}

std::shared_ptr<MirNativeBuffer> mcle::ClientBuffer::native_buffer_handle() const
{
    creation_package->age = age();
    return creation_package;
}

void mcle::ClientBuffer::update_from(MirBufferPackage const&)
{
}

void mcle::ClientBuffer::fill_update_msg(MirBufferPackage& package)
{
    package.data_items = 0;
    package.fd_items = 0;
}

MirNativeBuffer* mcle::ClientBuffer::as_mir_native_buffer() const
{
    //mesa has a POD native type for now. can return it directly to client API.
    return native_buffer_handle().get();
}

void mcle::ClientBuffer::set_fence(MirNativeFence*, MirBufferAccess)
{
}

MirNativeFence* mcle::ClientBuffer::get_fence() const
{
    return nullptr;
}

bool mcle::ClientBuffer::wait_fence(MirBufferAccess, std::chrono::nanoseconds)
{
    return true;
}
