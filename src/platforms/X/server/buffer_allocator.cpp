/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#include "buffer_allocator.h"
#include "../debug.h"

namespace mg  = mir::graphics;
namespace mgx = mg::X;
namespace geom = mir::geometry;

std::shared_ptr<mg::Buffer> mgx::BufferAllocator::alloc_buffer(
    BufferProperties const& /*buffer_properties*/)
{
#if 0
	std::shared_ptr<mg::Buffer> buffer;

    if (buffer_properties.usage == BufferUsage::software)
        buffer = alloc_software_buffer(buffer_properties);
    else
        buffer = alloc_hardware_buffer(buffer_properties);

    return buffer;
#endif
    CALLED
    return nullptr;
}

std::vector<MirPixelFormat> mgx::BufferAllocator::supported_pixel_formats()
{
    CALLED
    static std::vector<MirPixelFormat> const pixel_formats{
        mir_pixel_format_argb_8888,
        mir_pixel_format_xrgb_8888
    };

    return pixel_formats;
}
