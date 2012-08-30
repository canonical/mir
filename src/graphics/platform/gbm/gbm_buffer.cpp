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

#include <gbm.h>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace geom=mir::geometry;


geom::Width mgg::GBMBuffer::width() const
{
    return geom::Width(gbm_bo_get_width(gbm_handle));
}

geom::Height mgg::GBMBuffer::height() const
{
    return geom::Height(gbm_bo_get_height(gbm_handle));
}

geom::Stride mgg::GBMBuffer::stride() const
{
    return geom::Stride(gbm_bo_get_pitch(gbm_handle));
}

mc::PixelFormat mgg::GBMBuffer::pixel_format() const
{
    return gbm_format_to_mir_format(format);
}

void mgg::GBMBuffer::lock()
{

}

void mgg::GBMBuffer::unlock()
{

}

mg::Texture* mgg::GBMBuffer::bind_to_texture()
{
    return NULL;
}

mc::PixelFormat mgg::GBMBuffer::gbm_format_to_mir_format(gbm_bo_format format)
{
    (void)format;
    return mc::PixelFormat::rgba_8888;
}

gbm_bo_format mgg::GBMBuffer::mir_format_to_gbm_format(mc::PixelFormat format)
{
    (void)format;
    return GBM_BO_FORMAT_ARGB8888;
}
