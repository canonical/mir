/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "android_buffer_handle_default.h"
#include <system/window.h>

namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace geom=mir::geometry;


/* TODO: (kdub) this class has become trivial. remove soon */
mga::AndroidBufferHandleDefault::AndroidBufferHandleDefault(std::shared_ptr<ANativeWindowBuffer> const& buf, geom::PixelFormat pf, BufferUsage use)
    : anw_buffer(buf),
      pixel_format(pf),
      buffer_usage(use)
{
}

geom::Size mga::AndroidBufferHandleDefault::size() const
{
    return geom::Size{geom::Width{anw_buffer->width},
                      geom::Height{anw_buffer->height}};
}

geom::Stride mga::AndroidBufferHandleDefault::stride() const
{
    return geom::Stride(anw_buffer->stride);
}

geom::PixelFormat mga::AndroidBufferHandleDefault::format() const
{
    return pixel_format;
}

mga::BufferUsage mga::AndroidBufferHandleDefault::usage() const
{
    return buffer_usage;
}

std::shared_ptr<ANativeWindowBuffer> mga::AndroidBufferHandleDefault::native_buffer_handle() const
{
    return anw_buffer;
}
