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
#include "gbm_client_buffer.h"
#include "drm_fd_handler.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <stdexcept>

#include <xf86drm.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

namespace mcl=mir::client;
namespace mclg=mir::client::gbm;
namespace geom=mir::geometry;

namespace
{

struct GEMHandle
{
    GEMHandle(std::shared_ptr<mclg::DRMFDHandler> const& drm_fd_handler,
              int prime_fd)
        : drm_fd_handler{drm_fd_handler}
    {
        int ret = drm_fd_handler->primeFDToHandle(prime_fd, &handle);
        if (ret)
        {
            std::string msg("Failed to import PRIME fd for DRM buffer");
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(msg)) << boost::errinfo_errno(errno));
        }
    }

    ~GEMHandle()
    {
        struct drm_gem_close arg;
        arg.handle = handle;
        // TODO (@raof): Error reporting? I do not believe it should be possible for this to fail,
        //               so if it does we should probably flag it.
        drm_fd_handler->ioctl(DRM_IOCTL_GEM_CLOSE, &arg);
    }

    std::shared_ptr<mclg::DRMFDHandler> const drm_fd_handler;
    uint32_t handle;
};

struct NullDeleter
{
    void operator()(char *) const {}
};

struct GBMMemoryRegion : mcl::MemoryRegion
{
    GBMMemoryRegion(std::shared_ptr<mclg::DRMFDHandler> const& drm_fd_handler,
                    int prime_fd, geom::Size const& size_param,
                    geom::Stride stride_param, geom::PixelFormat format_param)
        : drm_fd_handler{drm_fd_handler},
          gem_handle{drm_fd_handler, prime_fd},
          size_in_bytes{size_param.height.as_uint32_t() * stride_param.as_uint32_t()}
    {
        width = size_param.width;
        height = size_param.height;
        stride = stride_param;
        format = format_param;

        struct drm_mode_map_dumb map_dumb;
        memset(&map_dumb, 0, sizeof(map_dumb));
        map_dumb.handle = gem_handle.handle;

        int ret = drm_fd_handler->ioctl(DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);
        if (ret)
        {
            std::string msg("Failed to map dumb DRM buffer");
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(msg)) << boost::errinfo_errno(errno));
        }

        void* map = drm_fd_handler->map(size_in_bytes, map_dumb.offset);
        if (map == MAP_FAILED)
        {
            std::string msg("Failed to mmap DRM buffer");
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(msg)) << boost::errinfo_errno(errno));
        }

        vaddr = std::shared_ptr<char>(static_cast<char*>(map), NullDeleter());
    }

    ~GBMMemoryRegion()
    {
        drm_fd_handler->unmap(vaddr.get(), size_in_bytes);
    }

    std::shared_ptr<mclg::DRMFDHandler> const drm_fd_handler;
    GEMHandle const gem_handle;
    size_t const size_in_bytes;
};

}

mclg::GBMClientBuffer::GBMClientBuffer(
        std::shared_ptr<mclg::DRMFDHandler> const& drm_fd_handler,
        std::shared_ptr<MirBufferPackage> const& package,
        geom::PixelFormat pf)
    : drm_fd_handler{drm_fd_handler},
      creation_package(std::move(package)),
      rect({geom::Point{0, 0}, geom::Size{package->width, package->height}}),
      buffer_pf{pf}
{
}

mclg::GBMClientBuffer::~GBMClientBuffer() noexcept
{
    // TODO (@raof): Error reporting? It should not be possible for this to fail; if it does,
    //               something's seriously wrong.
    drm_fd_handler->close(creation_package->fd[0]);
}

std::shared_ptr<mcl::MemoryRegion> mclg::GBMClientBuffer::secure_for_cpu_write()
{
    const int prime_fd = creation_package->fd[0];

    return std::make_shared<GBMMemoryRegion>(drm_fd_handler,
                                             prime_fd,
                                             size(),
                                             stride(),
                                             pixel_format());
}

geom::Size mclg::GBMClientBuffer::size() const
{
    return rect.size;
}

geom::Stride mclg::GBMClientBuffer::stride() const
{
    return geom::Stride{creation_package->stride};
}

geom::PixelFormat mclg::GBMClientBuffer::pixel_format() const
{
    return buffer_pf;
}

std::shared_ptr<MirNativeBuffer> mclg::GBMClientBuffer::native_buffer_handle() const
{
    creation_package->age = age();
    return creation_package;
}
