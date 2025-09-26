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
    std::shared_ptr<mir::renderer::software::RWMappable> data,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release) -> std::shared_ptr<mg::Buffer>
{
    auto buffer = std::make_shared<mg::common::NotifyingMappableBackedShmBuffer>(
        std::move(data),
        std::move(on_consumed),
        std::move(on_release));

    return buffer;
}
