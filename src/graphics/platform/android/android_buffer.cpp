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

#include "mir/graphics/android/android_buffer.h"

namespace mg=mir::graphics;
namespace geom=mir::geometry;

mg::AndroidBuffer::AndroidBuffer(std::shared_ptr<GraphicAllocAdaptor> alloc_dev,
                                 geom::Width w, geom::Height h, mc::PixelFormat pf)
    :
    buffer_width(w),
    buffer_height(h),
    buffer_format(pf),
    alloc_device(alloc_dev)
{
    bool ret;
    BufferUsage usage = mg::BufferUsage::use_hardware;

    if (!alloc_device)
        throw std::runtime_error("No allocation device for graphics buffer");

    ret = alloc_device->alloc_buffer( android_handle, buffer_stride,
                                      buffer_width, buffer_height,
                                      buffer_format, usage);
    if (ret)
        return;

    throw std::runtime_error("Graphics buffer allocation failed");
}

mg::AndroidBuffer::~AndroidBuffer()
{
}

geom::Width mg::AndroidBuffer::width() const
{
    return buffer_width;
}

geom::Height mg::AndroidBuffer::height() const
{
    return buffer_height;
}

geom::Stride mg::AndroidBuffer::stride() const
{
    return geom::Stride(0);
}

mc::PixelFormat mg::AndroidBuffer::pixel_format() const
{
    return buffer_format;
}

void mg::AndroidBuffer::lock()
{
}

void mg::AndroidBuffer::unlock()
{
}

mg::Texture* mg::AndroidBuffer::bind_to_texture()
{
    return NULL;
}
