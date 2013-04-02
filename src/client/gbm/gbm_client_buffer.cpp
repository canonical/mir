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
 * GNU General Public License for more details.
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
              uint32_t gem_name)
        : drm_fd_handler{drm_fd_handler}
    {
        struct drm_gem_open gem_open;
        memset(&gem_open, 0, sizeof(gem_open));
        gem_open.name = gem_name;

        int ret = drm_fd_handler->ioctl(DRM_IOCTL_GEM_OPEN, &gem_open);
        if (ret)
        {
            std::string msg("Failed to open GEM handle for DRM buffer");
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(msg)) << boost::errinfo_errno(errno));
        }

        handle = gem_open.handle;
    }

    ~GEMHandle()
    {
        static const uint32_t pad{0};
        struct drm_gem_close gem_close = {handle, pad};
        drm_fd_handler->ioctl(DRM_IOCTL_GEM_CLOSE, &gem_close);
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
                    uint32_t gem_name, geom::Size const& size_param,
                    geom::Stride stride_param, geom::PixelFormat format_param)
        : drm_fd_handler{drm_fd_handler},
          gem_handle{drm_fd_handler, gem_name},
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
        geom::Size size, geom::PixelFormat pf)
    : drm_fd_handler{drm_fd_handler},
      creation_package(std::move(package)),
      rect({{geom::X(0), geom::Y(0)}, size}),
      buffer_pf{pf}
{
}

std::shared_ptr<mcl::MemoryRegion> mclg::GBMClientBuffer::secure_for_cpu_write()
{
    const uint32_t gem_name = creation_package->data[0];

    return std::make_shared<GBMMemoryRegion>(drm_fd_handler,
                                             gem_name,
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

std::shared_ptr<MirBufferPackage> mclg::GBMClientBuffer::get_buffer_package() const
{
    creation_package->age = age();
    return creation_package;
}

MirNativeBuffer mclg::GBMClientBuffer::get_native_handle()
{
    return NULL;
}
