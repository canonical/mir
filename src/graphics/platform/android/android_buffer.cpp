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

mg::AndroidBuffer::AndroidBuffer(struct alloc_device_t *,
                 geom::Width w, geom::Height h, mc::PixelFormat pf)
 :
buffer_width(w),
buffer_height(h),
buffer_format(pf) 
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
