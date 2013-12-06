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
#include "client_buffer.h"
#include "buffer_file_ops.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <stdexcept>

#include <errno.h>
#include <sys/mman.h>

namespace mcl=mir::client;
namespace mclm=mir::client::mesa;
namespace geom=mir::geometry;

namespace
{

struct NullDeleter
{
    void operator()(char *) const {}
};

struct ShmMemoryRegion : mcl::MemoryRegion
{
    ShmMemoryRegion(std::shared_ptr<mclm::BufferFileOps> const& buffer_file_ops,
                    int buffer_fd, geom::Size const& size_param,
                    geom::Stride stride_param, MirPixelFormat format_param)
        : buffer_file_ops{buffer_file_ops},
          size_in_bytes{size_param.height.as_uint32_t() * stride_param.as_uint32_t()}
    {
        static off_t const map_offset = 0;
        width = size_param.width;
        height = size_param.height;
        stride = stride_param;
        format = format_param;

        void* map = buffer_file_ops->map(buffer_fd, map_offset, size_in_bytes);
        if (map == MAP_FAILED)
        {
            std::string msg("Failed to mmap buffer");
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(msg)) << boost::errinfo_errno(errno));
        }

        vaddr = std::shared_ptr<char>(static_cast<char*>(map), NullDeleter());
    }

    ~ShmMemoryRegion()
    {
        buffer_file_ops->unmap(vaddr.get(), size_in_bytes);
    }

    std::shared_ptr<mclm::BufferFileOps> const buffer_file_ops;
    size_t const size_in_bytes;
};

}

mclm::ClientBuffer::ClientBuffer(
    std::shared_ptr<mclm::BufferFileOps> const& buffer_file_ops,
    std::shared_ptr<MirBufferPackage> const& package,
    geom::Size size, MirPixelFormat pf)
    : buffer_file_ops{buffer_file_ops},
      creation_package{package},
      rect({geom::Point{0, 0}, size}),
      buffer_pf{pf}
{
}

mclm::ClientBuffer::~ClientBuffer() noexcept
{
    // TODO (@raof): Error reporting? It should not be possible for this to fail; if it does,
    //               something's seriously wrong.
    buffer_file_ops->close(creation_package->fd[0]);
}

std::shared_ptr<mcl::MemoryRegion> mclm::ClientBuffer::secure_for_cpu_write()
{
    int const buffer_fd = creation_package->fd[0];

    return std::make_shared<ShmMemoryRegion>(buffer_file_ops,
                                             buffer_fd,
                                             size(),
                                             stride(),
                                             pixel_format());
}

geom::Size mclm::ClientBuffer::size() const
{
    return rect.size;
}

geom::Stride mclm::ClientBuffer::stride() const
{
    return geom::Stride{creation_package->stride};
}

MirPixelFormat mclm::ClientBuffer::pixel_format() const
{
    return buffer_pf;
}

std::shared_ptr<MirNativeBuffer> mclm::ClientBuffer::native_buffer_handle() const
{
    creation_package->age = age();
    return creation_package;
}
