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

#include "mir/graphics/android/android_buffer_handle.h"

namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

mga::AndroidBufferHandle::AndroidBufferHandle(ANativeWindowBuffer buf, mc::PixelFormat pf, BufferUsage use)
 : anw_buffer(buf), 
   pixel_format(pf),
   buffer_usage(use)
{
}

EGLClientBuffer mga::AndroidBufferHandle::get_egl_client_buffer() const
{
    printf("calling here\n");
    return (EGLClientBuffer) &anw_buffer;
}

geom::Height mga::AndroidBufferHandle::height() const
{
    return geom::Height(anw_buffer.height);
}

geom::Width mga::AndroidBufferHandle::width() const
{
    return geom::Width(anw_buffer.width);
}

geom::Stride mga::AndroidBufferHandle::stride() const
{
    return geom::Stride(anw_buffer.stride);
}

mc::PixelFormat mga::AndroidBufferHandle::format() const
{
    return pixel_format; 
}

mga::BufferUsage mga::AndroidBufferHandle::usage() const
{
    return buffer_usage;
}
