/*
 * Copyright © 2019 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_RPI_VC4_BUFFER_ALLOCATOR_H_
#define MIR_GRAPHICS_RPI_VC4_BUFFER_ALLOCATOR_H_

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/wayland_allocator.h"
#include "mir/graphics/egl_extensions.h"

#include <interface/vmcs_host/vc_dispmanx_types.h>

namespace mir
{
class Executor;

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

namespace common
{
class EGLContextExecutor;
}

namespace rpi
{
class DispmanXBuffer
{
public:
    virtual ~DispmanXBuffer() = default;

    virtual explicit operator DISPMANX_RESOURCE_HANDLE_T() const = 0;
    virtual DISPMANX_TRANSFORM_T resource_transform() const = 0;
};

class BufferAllocator :
	public GraphicBufferAllocator,
	public WaylandAllocator
{
public:
    BufferAllocator(graphics::Display const& output);

    std::shared_ptr<Buffer> alloc_buffer(BufferProperties const &buffer_properties) override;
    std::vector<MirPixelFormat> supported_pixel_formats() override;
    std::shared_ptr<Buffer> alloc_buffer(geometry::Size size, uint32_t native_format, uint32_t native_flags) override;
    std::shared_ptr<Buffer> alloc_software_buffer(geometry::Size size, MirPixelFormat format) override;

    void bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor) override;
    std::shared_ptr<Buffer> buffer_from_resource(
        wl_resource* resource,
	std::function<void()>&& ,
	std::function<void()>&&) override;

    std::shared_ptr<Buffer> buffer_from_shm(wl_resource* buffer, std::shared_ptr<mir::Executor> wayland_executor,
                                            std::function<void()>&& on_consumed) override;

private:
    std::shared_ptr<EGLExtensions> const egl_extensions;
    std::shared_ptr<renderer::gl::Context> const ctx;
    std::shared_ptr<common::EGLContextExecutor> const egl_executor;
    std::shared_ptr<Executor> wayland_executor;
};
}
}
}
#endif  // MIR_GRAPHICS_RPI_VC4_BUFFER_ALLOCATOR_H_
