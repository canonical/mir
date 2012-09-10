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

#include "mir/graphics/platform.h"
#include "mir/graphics/gbm/gbm_buffer_allocator.h"
#include "mir/graphics/gbm/gbm_buffer.h"

#include <stdexcept>
#include <xf86drm.h>

namespace mg  = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mc  = mir::compositor;
namespace geom = mir::geometry;

mgg::GBMBufferAllocator::GBMBufferAllocator(const std::shared_ptr<gbm_device>& dev)
        : dev(dev)
{
}

std::unique_ptr<mc::Buffer> mgg::GBMBufferAllocator::alloc_buffer(
    geom::Width width, geom::Height height, mc::PixelFormat pf)
{
    gbm_bo *handle = gbm_bo_create(
        dev.get(), 
        width.as_uint32_t(), 
        height.as_uint32_t(),
        mgg::mir_format_to_gbm_format(pf), 
        GBM_BO_USE_RENDERING);
    
    if (handle != NULL)
        return std::unique_ptr<mc::Buffer>(new GBMBuffer(std::unique_ptr<gbm_bo, mgg::GBMBufferObjectDeleter>(handle)));

    return std::unique_ptr<mc::Buffer>();
}

namespace
{

struct GbmDeviceDeleter
{
    void operator()(gbm_device* dev) const
    {
        if (dev)
            gbm_device_destroy(dev);
    }
};

std::shared_ptr<gbm_device> create_drm_device()
{
    /*
     * TODO:
     *  - We need an API to actually select the right driver here
     *  - We need to fix libgbm to support out-of-tree backends, then write and use a software-only backend
     *    that we can load for the tests.
     *
     * For the moment, just ignore the fact that we're not actually creating a usable device.
     */

    int drm_fd = drmOpen("radeon", NULL);
    
    /* TODO: Enable error handling once we can load the correct backend.
    if (drm_fd < 0) {
        throw std::runtime_error("Failed to open drm device");
    }
    */
    
    gbm_device* dev = gbm_create_device(drm_fd);

    /* TODO: Enable error handling once we can load the correct backend.    
    if (!dev) {
        throw std::runtime_error("Failed to create gbm device");
    }
    */

    return std::shared_ptr<gbm_device>(dev, GbmDeviceDeleter());
}
}

std::shared_ptr<mc::GraphicBufferAllocator> mg::create_buffer_allocator()
{
    return std::make_shared<mgg::GBMBufferAllocator>(create_drm_device());
}
