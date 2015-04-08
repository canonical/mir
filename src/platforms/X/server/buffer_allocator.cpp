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
#include "anonymous_shm_file.h"
#include "shm_buffer.h"
#include "mir/graphics/buffer_properties.h"
#include <boost/throw_exception.hpp>
#include <algorithm>

#include "../debug.h"

namespace mg  = mir::graphics;
namespace mgx = mg::X;
namespace geom = mir::geometry;

std::shared_ptr<mg::Buffer> mgx::BufferAllocator::alloc_buffer(
    BufferProperties const& buffer_properties)
{
	std::shared_ptr<mg::Buffer> buffer;

    CALLED
    if (buffer_properties.usage == BufferUsage::software)
    {
        std::cout<< "\t\t software buffer" << std::endl;
        buffer = alloc_software_buffer(buffer_properties);
    }
    else
    {
        std::cout<< "\t\t hardware buffer" << std::endl;
        buffer = alloc_hardware_buffer(buffer_properties);
//        return nullptr;
    }

    return buffer;
}

std::shared_ptr<mg::Buffer> mgx::BufferAllocator::alloc_hardware_buffer(
    BufferProperties const& buffer_properties)
{
    return alloc_software_buffer(buffer_properties);
}

std::shared_ptr<mg::Buffer> mgx::BufferAllocator::alloc_software_buffer(
    BufferProperties const& buffer_properties)
{
    if (!is_pixel_format_supported(buffer_properties.format))
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Trying to create SHM buffer with unsupported pixel format"));
    }

    auto const stride = geom::Stride{
        MIR_BYTES_PER_PIXEL(buffer_properties.format) *
        buffer_properties.size.width.as_uint32_t()};
    size_t const size_in_bytes =
        stride.as_int() * buffer_properties.size.height.as_int();
    auto const shm_file =
        std::make_shared<mgx::AnonymousShmFile>(size_in_bytes);

    auto const buffer =
        std::make_shared<ShmBuffer>(shm_file, buffer_properties.size,
                                    buffer_properties.format);

    return buffer;
}

std::vector<MirPixelFormat> mgx::BufferAllocator::supported_pixel_formats()
{
    CALLED
    static std::vector<MirPixelFormat> const pixel_formats{
        mir_pixel_format_argb_8888,
        mir_pixel_format_xrgb_8888,
        mir_pixel_format_bgr_888
    };

    return pixel_formats;
}

bool mgx::BufferAllocator::is_pixel_format_supported(MirPixelFormat format)
{
    auto formats = supported_pixel_formats();

    auto iter = std::find(formats.begin(), formats.end(), format);

    return iter != formats.end();
}
