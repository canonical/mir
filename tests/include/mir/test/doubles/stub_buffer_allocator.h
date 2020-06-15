/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_
#define MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir_test_framework/stub_platform_native_buffer.h"
#include "mir_toolkit/client_types.h"
#include "src/platforms/common/server/buffer_from_wl_shm.h"
#include "src/platforms/common/server/egl_context_executor.h"
#include "mir/test/doubles/null_gl_context.h"
#include <wayland-server.h>

#include <vector>
#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

namespace
{
void memcpy_from_shm_buffer(struct wl_shm_buffer* buffer)
{
    auto const height = wl_shm_buffer_get_height(buffer);
    auto const stride = wl_shm_buffer_get_stride(buffer);
    auto dummy_destination = std::make_unique<unsigned char[]>(height * stride);

    wl_shm_buffer_begin_access(buffer);
    ::memcpy(dummy_destination.get(), wl_shm_buffer_get_data(buffer), height * stride);
    wl_shm_buffer_end_access(buffer);
}
}

struct StubBufferAllocator : public graphics::GraphicBufferAllocator
{
    std::shared_ptr<graphics::Buffer> alloc_software_buffer(geometry::Size sz, MirPixelFormat pf) override
    {
        graphics::BufferProperties properties{sz, pf, graphics::BufferUsage::software};
        return std::make_shared<StubBuffer>(std::make_shared<mir_test_framework::NativeBuffer>(properties), properties, geometry::Stride{sz.width.as_uint32_t() * MIR_BYTES_PER_PIXEL(pf)});
    }

    std::vector<MirPixelFormat> supported_pixel_formats() override
    {
        return { mir_pixel_format_argb_8888 };
    }

    void bind_display(wl_display*, std::shared_ptr<mir::Executor>) override
    {
    }

    auto buffer_from_resource(wl_resource*, std::function<void()>&&, std::function<void()>&&)
        -> std::shared_ptr<graphics::Buffer> override
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"StubBufferAllocator doesn't do HW Wayland buffers"}));
    }

    auto buffer_from_shm(
        wl_resource* resource,
        std::shared_ptr<mir::Executor> executor,
        std::function<void()>&& on_consumed) -> std::shared_ptr<graphics::Buffer> override
    {
        // Temporary(?!) hack to actually use the buffer, for WLCS test
        // Transitioning the StubGraphicsPlatform to use the MESA surfaceless GL platform would
        // allow us to test more of Mir, and drop this hack
        memcpy_from_shm_buffer(wl_shm_buffer_get(resource));

        return graphics::wayland::buffer_from_wl_shm(
            resource,
            std::move(executor),
            std::make_shared<graphics::common::EGLContextExecutor>(std::make_unique<test::doubles::NullGLContext>()),
            std::move(on_consumed));
    }
};

}
}
}

#endif // MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_
