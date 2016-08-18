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

#include "host_connection.h"
#include "mir_toolkit/mir_buffer.h"
#include "buffer.h"
#include <string.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

mgn::Buffer::Buffer(
    std::shared_ptr<HostConnection> const& connection,
    mg::BufferProperties const& properties) :
    connection(connection),
    buffer(connection->create_buffer(properties))
{
}

std::shared_ptr<mg::NativeBuffer> mgn::Buffer::native_buffer_handle() const
{
    return nullptr;
}

geom::Size mgn::Buffer::size() const
{
    return geom::Size{ mir_buffer_get_width(buffer.get()), mir_buffer_get_height(buffer.get()) };
}

geom::Stride mgn::Buffer::stride() const
{
    return geom::Stride{mir_buffer_get_stride(buffer.get())};
}

MirPixelFormat mgn::Buffer::pixel_format() const
{
    return mir_buffer_get_pixel_format(buffer.get());
}

void mgn::Buffer::write(unsigned char const* pixels, size_t pixel_size)
{
    auto bpp = MIR_BYTES_PER_PIXEL(pixel_format());
    size_t buffer_size_bytes = size().height.as_int() * size().width.as_int() * bpp;
    if (buffer_size_bytes != pixel_size)
        BOOST_THROW_EXCEPTION(std::logic_error("Size of pixels is not equal to size of buffer"));

    auto region = connection->get_graphics_region(buffer.get());
    for (int i = 0; i < region.height; i++)
    {
        int line_offset_in_buffer = stride().as_uint32_t()*i;
        int line_offset_in_source = bpp * region.width * i;
        memcpy(region.vaddr + line_offset_in_buffer, pixels + line_offset_in_source, region.width * bpp);
    }
}

void mgn::Buffer::read(std::function<void(unsigned char const*)> const& do_with_pixels)
{
    auto region = connection->get_graphics_region(buffer.get());
    do_with_pixels(reinterpret_cast<unsigned char*>(region.vaddr));
}

mg::NativeBufferBase* mgn::Buffer::native_buffer_base()
{
    return nullptr;
}
