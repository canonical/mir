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

#include "mir/graphics/gbm/gbm_buffer_allocator.h"
#include "mir/graphics/gbm/gbm_buffer.h"
#include "mir/graphics/gbm/gbm_platform.h"

#include <stdexcept>
#include <xf86drm.h>
#include <gbm.h>

namespace mg  = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mc  = mir::compositor;
namespace geom = mir::geometry;

mgg::GBMBufferAllocator::GBMBufferAllocator(const std::shared_ptr<GBMPlatform>& platform)
        : platform(platform)
{
}

std::unique_ptr<mc::Buffer> mgg::GBMBufferAllocator::alloc_buffer(
    geom::Size size, mc::PixelFormat pf)
{
    gbm_bo *handle = gbm_bo_create(
        platform->gbm.device, 
        size.width.as_uint32_t(), 
        size.height.as_uint32_t(),
        mgg::mir_format_to_gbm_format(pf), 
        GBM_BO_USE_RENDERING);
    
    if (handle != NULL)
        return std::unique_ptr<mc::Buffer>(new GBMBuffer(std::unique_ptr<gbm_bo, mgg::GBMBufferObjectDeleter>(handle)));

    return std::unique_ptr<mc::Buffer>();
}
