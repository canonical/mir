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

#include "mir/graphics/gbm/gbm_buffer.h"
#include "mir/compositor/buffer_ipc_package.h"

#include <gbm.h>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace geom=mir::geometry;

void mgg::GBMBufferObjectDeleter::operator()(gbm_bo* handle) const
{
    if (handle)
        gbm_bo_destroy(handle);
}

geom::PixelFormat mgg::gbm_format_to_mir_format(uint32_t format)
{
    (void)format;
    return geom::PixelFormat::rgba_8888;
}

uint32_t mgg::mir_format_to_gbm_format(geom::PixelFormat format)
{
    (void)format;
    return GBM_BO_FORMAT_ARGB8888;
}


mgg::GBMBuffer::GBMBuffer(
    std::unique_ptr<gbm_bo, mgg::GBMBufferObjectDeleter> handle) 
        : gbm_handle(std::move(handle))
{
}

mgg::GBMBuffer::~GBMBuffer()
{
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
    temp->ipc_data.push_back(gbm_bo_get_handle(gbm_handle.get()).u32);
    return temp;
}

void mgg::GBMBuffer::bind_to_texture()
{
}
