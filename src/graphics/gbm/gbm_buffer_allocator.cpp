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
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "gbm_buffer_allocator.h"
#include "gbm_buffer.h"
#include "gbm_platform.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/exception.h"

#include <stdexcept>
#include <xf86drm.h>
#include <gbm.h>
#include <cassert>

namespace mg  = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mc  = mir::compositor;
namespace geom = mir::geometry;

mgg::GBMBufferAllocator::GBMBufferAllocator(
        const std::shared_ptr<GBMPlatform>& platform,
        const std::shared_ptr<BufferInitializer>& buffer_initializer)
        : platform(platform), buffer_initializer(buffer_initializer)
{
    assert(buffer_initializer.get() != 0);
}

std::unique_ptr<mc::Buffer> mgg::GBMBufferAllocator::alloc_buffer(
    mc::BufferProperties const& buffer_properties)
{
    uint32_t bo_flags{0};

    /* Create the GBM buffer object */
    if (buffer_properties.usage == mc::BufferUsage::software)
        bo_flags |= GBM_BO_USE_WRITE;
    else
        bo_flags |= GBM_BO_USE_RENDERING;

    gbm_bo *bo_raw = gbm_bo_create(
        platform->gbm.device, 
        buffer_properties.size.width.as_uint32_t(),
        buffer_properties.size.height.as_uint32_t(),
        mgg::mir_format_to_gbm_format(buffer_properties.format),
        bo_flags);
    
    if (!bo_raw)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create GBM buffer object"));

    std::unique_ptr<gbm_bo, mgg::GBMBufferObjectDeleter> bo{bo_raw};

    /* Get the GEM flink name from the GBM buffer object */
    auto gem_handle = gbm_bo_get_handle(bo_raw).u32;
    auto drm_fd = gbm_device_get_fd(platform->gbm.device);
    struct drm_gem_flink flink;
    flink.handle = gem_handle;

    auto ret = drmIoctl(drm_fd, DRM_IOCTL_GEM_FLINK, &flink);
    if (ret)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to get GEM flink name from gbm bo"));

    /* Create the GBMBuffer */
    std::unique_ptr<mc::Buffer> buffer{new GBMBuffer{std::move(bo), flink.name}};

    (*buffer_initializer)(*buffer);

    return buffer;
}
