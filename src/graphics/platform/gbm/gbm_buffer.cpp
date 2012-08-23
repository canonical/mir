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
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/gbm/gbm_buffer.h"

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace geom=mir::geometry;


mgg::GBMBuffer::GBMBuffer(const std::shared_ptr<GraphicAllocAdaptor>& device,
                          geom::Width w, geom::Height h, mc::PixelFormat pf)
    :
    buffer_width(w),
    buffer_height(h),
    buffer_format(pf),
    alloc_device(device)
{
    bool ret;
    BufferUsage usage = mg::BufferUsage::use_hardware;

    if (!alloc_device)
        throw std::runtime_error("No allocation device for graphics buffer");

    ret = alloc_device->alloc_buffer( gbm_handle, buffer_stride,
                                      buffer_width, buffer_height,
                                      buffer_format, usage);
    if (ret)
        return;

    throw std::runtime_error("Graphics buffer allocation failed");
}

mgg::GBMBuffer::~GBMBuffer()
{

}

geom::Width mgg::GBMBuffer::width() const
{
    return buffer_width;
}

geom::Height mgg::GBMBuffer::height() const
{
    return buffer_height;
}

geom::Stride mgg::GBMBuffer::stride() const
{
    return geom::Stride(0);
}

mc::PixelFormat mgg::GBMBuffer::pixel_format() const
{
    return buffer_format;
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
