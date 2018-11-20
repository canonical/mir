/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include "mir/anonymous_shm_file.h"
#include "shm_buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/display.h"
#include "software_buffer.h"
#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <system_error>
#include <cassert>

namespace mg  = mir::graphics;
namespace mge = mg::eglstream;
namespace mgc = mg::common;
namespace geom = mir::geometry;

namespace
{
std::unique_ptr<mir::renderer::gl::Context> context_for_output(mg::Display const& output)
{
    try
    {
        auto& context_source = dynamic_cast<mir::renderer::gl::ContextSource const&>(output);

        /*
         * We care about no part of this context's config; we will do no rendering with it.
         * All we care is that we can allocate texture IDs and bind a texture, which is
         * config independent.
         *
         * That's not *entirely* true; we also need it to be on the same device as we want
         * to do the rendering on, and that GL must support all the extensions we care about,
         * but since we don't yet support heterogeneous hybrid and implementing that will require
         * broader interface changes it's a safe enough requirement for now.
         */
        return context_source.create_gl_context();
    }
    catch (std::bad_cast const& err)
    {
        std::throw_with_nested(
            boost::enable_error_info(
                std::runtime_error{"Output platform cannot provide a GL context"})
                << boost::throw_function(__PRETTY_FUNCTION__)
                << boost::throw_line(__LINE__)
                << boost::throw_file(__FILE__));
    }
}
}

mge::BufferAllocator::BufferAllocator(mg::Display const& output)
    : ctx{context_for_output(output)}
{
}

mge::BufferAllocator::~BufferAllocator() = default;

std::shared_ptr<mg::Buffer> mge::BufferAllocator::alloc_buffer(
    BufferProperties const& buffer_properties)
{
    if (buffer_properties.usage == mg::BufferUsage::software)
        return alloc_software_buffer(buffer_properties.size, buffer_properties.format);
    BOOST_THROW_EXCEPTION(std::runtime_error("platform incapable of creating hardware buffers"));
}

std::shared_ptr<mg::Buffer> mge::BufferAllocator::alloc_software_buffer(geom::Size size, MirPixelFormat format)
{
    if (!mgc::ShmBuffer::supports(format))
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Trying to create SHM buffer with unsupported pixel format"));
    }

    auto const stride = geom::Stride{ MIR_BYTES_PER_PIXEL(format) * size.width.as_uint32_t() };
    size_t const size_in_bytes = stride.as_int() * size.height.as_int();
    return std::make_shared<mge::SoftwareBuffer>(
        std::make_unique<mir::AnonymousShmFile>(size_in_bytes), size, format);
}

std::vector<MirPixelFormat> mge::BufferAllocator::supported_pixel_formats()
{
    // Lazy
    return {mir_pixel_format_argb_8888, mir_pixel_format_xrgb_8888};
}

std::shared_ptr<mg::Buffer> mge::BufferAllocator::alloc_buffer(geometry::Size, uint32_t, uint32_t)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("platform incapable of creating buffers"));
} 
