/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_BUFFER_ALLOCATOR_H_
#define MIR_GRAPHICS_MESA_BUFFER_ALLOCATOR_H_

#include "platform_common.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/wayland_allocator.h"
#include "mir_toolkit/mir_native_buffer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wall"
#include <gbm.h>
#pragma GCC diagnostic pop

#include <EGL/egl.h>
#include <wayland-server-core.h>

#include <memory>

namespace mir
{
namespace renderer
{
namespace gl
{
class Context;
}
}
namespace graphics
{
class Display;
struct EGLExtensions;

namespace mesa
{

enum class BufferImportMethod
{
    gbm_native_pixmap,
    dma_buf
};

class BufferAllocator:
    public graphics::GraphicBufferAllocator,
    public graphics::WaylandAllocator
{
public:
    BufferAllocator(
        Display const& output,
        gbm_device* device,
        BypassOption bypass_option,
        BufferImportMethod const buffer_import_method);

    std::shared_ptr<Buffer> alloc_buffer(
        geometry::Size size, uint32_t native_format, uint32_t native_flags) override;
    std::shared_ptr<Buffer> alloc_software_buffer(geometry::Size size, MirPixelFormat) override;
    std::shared_ptr<Buffer> alloc_buffer(graphics::BufferProperties const& buffer_properties) override;
    std::vector<MirPixelFormat> supported_pixel_formats() override;

    void bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor) override;
    std::shared_ptr<Buffer> buffer_from_resource(
        wl_resource* buffer,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) override;
private:
    std::shared_ptr<Buffer> alloc_hardware_buffer(
        graphics::BufferProperties const& buffer_properties);

    std::shared_ptr<renderer::gl::Context> const ctx;
    std::shared_ptr<Executor> wayland_executor;
    gbm_device* const device;
    std::shared_ptr<EGLExtensions> const egl_extensions;

    BypassOption const bypass_option;
    BufferImportMethod const buffer_import_method;
};

}
}
}

#endif // MIR_GRAPHICS_MESA_BUFFER_ALLOCATOR_H_
