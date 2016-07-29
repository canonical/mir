/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "buffer.h"

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

mgn::Buffer::Buffer(
    std::shared_ptr<HostConnection> const& connection,
    mg::BufferProperties const& properties) :
    connection(connection),
    properties(properties)
{
}

std::shared_ptr<mg::NativeBuffer> mgn::Buffer::native_buffer_handle() const
{
    return nullptr;
}

geom::Size mgn::Buffer::size() const
{
    return {};
}

geom::Stride mgn::Buffer::stride() const
{
    return {};
}

MirPixelFormat mgn::Buffer::pixel_format() const
{
    return mir_pixel_format_invalid;
}

void mgn::Buffer::write(unsigned char const* pixels, size_t size)
{
    (void)pixels;(void)size;
}

void mgn::Buffer::read(std::function<void(unsigned char const*)> const& do_with_pixels)
{
    (void) do_with_pixels;
}

mg::NativeBufferBase* mgn::Buffer::native_buffer_base()
{
    return nullptr;
}
