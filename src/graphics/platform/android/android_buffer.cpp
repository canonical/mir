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

#include "mir_platform/android/android_buffer.h"

namespace mg=mir::graphics;
namespace geom=mir::geometry;

mg::AndroidBuffer::AndroidBuffer(struct alloc_device_t * alloc_dev,
                 geom::Width w, geom::Height h, mc::PixelFormat pf)
 :
buffer_width(w),
buffer_height(h),
buffer_format(pf),
alloc_device(alloc_dev)
{
    int ret;

    if ((!alloc_device) || (!alloc_device->alloc) || (!alloc_device->free))
        throw std::runtime_error("No allocation device for graphics buffer");


    int format, usage, stride = 0;

    format = convert_to_android_format(pf);
    usage = 0x300;

    ret = alloc_device->alloc(
                        /* alloc_device is guaranteed by C library to remain same */
                        const_cast<struct alloc_device_t*> (alloc_device),
                        (int) buffer_width.as_uint32_t(),
                        (int) buffer_height.as_uint32_t(),
                        format, usage, &android_handle, &stride); 
    buffer_stride = geom::Stride(stride);

    if (ret == 0)
        return;
    else
        throw std::runtime_error("Graphics buffer allocation failed");
}


mg::AndroidBuffer::~AndroidBuffer()
{
    alloc_device->free( const_cast<struct alloc_device_t*> (alloc_device),
                        android_handle);
}

int mg::AndroidBuffer::convert_to_android_format(mc::PixelFormat pf)
{
    switch (pf)
    {
        case mc::PixelFormat::rgba_8888:
            return 4;
        default:
            /* will cause gralloc to error, exception thrown */
            return -1;
    }
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
