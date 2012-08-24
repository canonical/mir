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

namespace mg  = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mc  = mir::compositor;
namespace geom = mir::geometry;

mgg::GBMBufferAllocator::GBMBufferAllocator()
{

}

std::unique_ptr<mc::Buffer> mgg::GBMBufferAllocator::alloc_buffer(
    geom::Width width, geom::Height height, mc::PixelFormat pf)
{
    (void)pf;
    struct gbm_bo *handle = gbm_bo_create(dev.get(), width.as_uint32_t(), height.as_uint32_t(), GBM_BO_FORMAT_ARGB8888, 0);
    if (handle != NULL)
        return std::unique_ptr<mc::Buffer>(new GBMBuffer(handle));

    return std::unique_ptr<mc::Buffer>();
}

std::unique_ptr<mc::GraphicBufferAllocator> mg::create_buffer_allocator()
{
    return std::unique_ptr<mgg::GBMBufferAllocator>(new mgg::GBMBufferAllocator());
}
