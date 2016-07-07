/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "buffer_allocator.h"
#include "buffer_texture_binder.h"
#include "anonymous_shm_file.h"
#include "shm_buffer.h"
#include "mir/graphics/buffer_properties.h"
#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <system_error>
#include <cassert>

namespace mg  = mir::graphics;
namespace mge = mg::eglstream;
namespace mgc = mg::common;
namespace geom = mir::geometry;

mge::BufferAllocator::BufferAllocator()
{
}

std::shared_ptr<mg::Buffer> mge::BufferAllocator::alloc_buffer(
    BufferProperties const& buffer_properties)
{
    if (!mgc::ShmBuffer::supports(buffer_properties.format))
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
    auto shm_file =
        std::make_unique<mgc::AnonymousShmFile>(size_in_bytes);

    auto const buffer =
        std::make_shared<mgc::ShmBuffer>(std::move(shm_file), buffer_properties.size,
                                         buffer_properties.format);

    return buffer;
}

std::vector<MirPixelFormat> mge::BufferAllocator::supported_pixel_formats()
{
    // Lazy
    return {mir_pixel_format_argb_8888, mir_pixel_format_xrgb_8888};
}
