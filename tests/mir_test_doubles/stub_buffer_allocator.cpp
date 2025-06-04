/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_buffer.h"
#include "src/platforms/common/server/shm_buffer.h"
#include "mir/graphics/egl_context_executor.h"
#include "mir/test/doubles/null_gl_context.h"
#include "mir/renderer/sw/pixel_source.h"
#include <wayland-server.h>

#include <vector>
#include <memory>

#include <boost/throw_exception.hpp>

namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;

namespace
{
/*
 * Oh, no.
 *
 * Testing that we correctly handle bad Shm buffers sent from clients requires that
 * we *actually read* from the buffer so that the kernel can generate an access fault.
 * To that end, we `memcpy` from the submitted buffer to a bit of scratch memory in
 * order to read every byte of the submitted buffer.
 *
 * Unfortunately, the optimiser can now see that we don't *do anything* with the
 * scratch memory, and so is now deciding to optimise out the memcpy.
 *
 * Rather than try to obfuscate the code enough that the optimiser can't prove that
 * we don't do anything with the contents of `buffer`, just annotate the function
 * with a “kindly don't optimise this” attribute.
 *
 * Of course, this isn't a standardised attribute, so apply the gcc *and* the clang
 * one. And if we need to build with another compiler...
 */
[[clang::optnone, gnu::optimize(0)]]
inline void memcpy_from_mapping(mir::renderer::software::ReadMappableBuffer& buffer)
{
    auto const mapping = buffer.map_readable();
    auto dummy_destination = std::make_unique<unsigned char[]>(mapping->len());

    memcpy(dummy_destination.get(), mapping->data(), mapping->len());
}
}

auto mtd::StubBufferAllocator::alloc_software_buffer(geometry::Size sz, MirPixelFormat pf) -> std::shared_ptr<mg::Buffer>
{
    graphics::BufferProperties properties{sz, pf, graphics::BufferUsage::software};
    return std::make_shared<StubBuffer>(properties, geometry::Stride{sz.width.as_uint32_t() * MIR_BYTES_PER_PIXEL(pf)});
}

auto mtd::StubBufferAllocator::supported_pixel_formats() -> std::vector<MirPixelFormat>
{
    return { mir_pixel_format_argb_8888 };
}

void mtd::StubBufferAllocator::bind_display(wl_display*, std::shared_ptr<mir::Executor>)
{
}

void mtd::StubBufferAllocator::unbind_display(wl_display*)
{
}

auto mtd::StubBufferAllocator::buffer_from_resource(wl_resource*, std::function<void()>&&, std::function<void()>&&)
    -> std::shared_ptr<mg::Buffer>
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"StubBufferAllocator doesn't do HW Wayland buffers"}));
}

auto mtd::StubBufferAllocator::buffer_from_shm(
    std::shared_ptr<mir::renderer::software::RWMappableBuffer> data,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release) -> std::shared_ptr<mg::Buffer>
{
    auto buffer = std::make_shared<mg::common::NotifyingMappableBackedShmBuffer>(
        std::move(data),
        std::move(on_consumed),
        std::move(on_release));

    // Temporary(?!) hack to actually use the buffer, for WLCS test
    // Transitioning the StubGraphicsPlatform to use the MESA surfaceless GL platform would
    // allow us to test more of Mir, and drop this hack
    memcpy_from_mapping(*buffer);

    return buffer;
}
