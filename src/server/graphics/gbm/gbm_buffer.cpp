/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gbm_buffer.h"
#include "buffer_texture_binder.h"
#include "mir/compositor/buffer_ipc_package.h"

#include <fcntl.h>
#include <xf86drm.h>

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <limits>
#include <stdexcept>

namespace mc=mir::compositor;
namespace mgg=mir::graphics::gbm;
namespace geom=mir::geometry;

geom::PixelFormat mgg::gbm_format_to_mir_format(uint32_t format)
{
    geom::PixelFormat pf;

    switch (format)
    {
    case GBM_BO_FORMAT_ARGB8888:
    case GBM_FORMAT_ARGB8888:
        pf = geom::PixelFormat::argb_8888;
        break;
    case GBM_BO_FORMAT_XRGB8888:
    case GBM_FORMAT_XRGB8888:
        pf = geom::PixelFormat::xrgb_8888;
        break;
    default:
        pf = geom::PixelFormat::invalid;
        break;
    }

    return pf;
}

uint32_t mgg::mir_format_to_gbm_format(geom::PixelFormat format)
{
    uint32_t gbm_pf;

    switch (format)
    {
    case geom::PixelFormat::argb_8888:
        gbm_pf = GBM_FORMAT_ARGB8888;
        break;
    case geom::PixelFormat::xrgb_8888:
        gbm_pf = GBM_FORMAT_XRGB8888;
        break;
    default:
        /* There is no explicit invalid GBM pixel format! */
        gbm_pf = std::numeric_limits<uint32_t>::max();
        break;
    }

    return gbm_pf;
}

mgg::GBMBuffer::GBMBuffer(std::shared_ptr<gbm_bo> const& handle,
                          std::unique_ptr<BufferTextureBinder> texture_binder)
    : gbm_handle{handle}, texture_binder{std::move(texture_binder)}
{
    auto device = gbm_bo_get_device(gbm_handle.get());
    auto gem_handle = gbm_bo_get_handle(gbm_handle.get()).u32;
    auto drm_fd = gbm_device_get_fd(device);

    auto ret = drmPrimeHandleToFD(drm_fd, gem_handle, DRM_CLOEXEC, &prime_fd);    

    if (ret)
    {
        std::string const msg("Failed to get PRIME fd from gbm bo");
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error(msg)) << boost::errinfo_errno(errno));
    }
}

geom::Size mgg::GBMBuffer::size() const
{
    return {geom::Width{gbm_bo_get_width(gbm_handle.get())},
            geom::Height{gbm_bo_get_height(gbm_handle.get())}};
}

geom::Stride mgg::GBMBuffer::stride() const
{
    return geom::Stride(gbm_bo_get_stride(gbm_handle.get()));
}

geom::PixelFormat mgg::GBMBuffer::pixel_format() const
{
    return gbm_format_to_mir_format(gbm_bo_get_format(gbm_handle.get()));
}

std::shared_ptr<mc::BufferIPCPackage> mgg::GBMBuffer::get_ipc_package() const
{
    auto temp = std::make_shared<mc::BufferIPCPackage>();

    temp->ipc_fds.push_back(prime_fd);
    temp->stride = stride().as_uint32_t();

    return temp;
}

void mgg::GBMBuffer::bind_to_texture()
{
    texture_binder->bind_to_texture();
}

std::shared_ptr<MirNativeBuffer> mgg::GBMBuffer::native_buffer_handle() const
{
    return std::make_shared<MirNativeBuffer>();
}
