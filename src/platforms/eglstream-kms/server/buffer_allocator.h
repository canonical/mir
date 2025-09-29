/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_PLATFORMS_EGLSTREAM_BUFFER_ALLOCATOR_
#define MIR_PLATFORMS_EGLSTREAM_BUFFER_ALLOCATOR_

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/egl_context_executor.h"
#include "mir/graphics/platform.h"

#include "wayland-eglstream-controller.h"

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

namespace gl
{
class Program;
}

namespace eglstream
{

class BufferAllocator:
    public graphics::GraphicBufferAllocator
{
public:
    BufferAllocator(std::unique_ptr<renderer::gl::Context> ctx);
    ~BufferAllocator();

    std::shared_ptr<Buffer> alloc_software_buffer(geometry::Size size, MirPixelFormat format) override;
    std::vector<MirPixelFormat> supported_pixel_formats() override;

    void bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor) override;
    void unbind_display(wl_display* display) override;

    std::shared_ptr<Buffer> buffer_from_resource(
        wl_resource* buffer,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) override;

    auto buffer_from_shm(
        std::shared_ptr<renderer::software::RWMappable> shm_data,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) -> std::shared_ptr<Buffer> override;

private:
    static void create_buffer_eglstream_resource(
        wl_client* client,
        wl_resource* eglstream_controller,
        wl_resource* surface,
        wl_resource* buffer);
    static void bind_eglstream_controller(
        wl_client* client,
        void* ctx,
        uint32_t version,
        uint32_t id);

    EGLExtensions::LazyDisplayExtensions<EGLExtensions::WaylandExtensions> const extensions;
    EGLExtensions::LazyDisplayExtensions<EGLExtensions::NVStreamAttribExtensions> const nv_extensions;
    std::shared_ptr<renderer::gl::Context> const wayland_ctx;
    std::shared_ptr<common::EGLContextExecutor> const egl_delegate;
    std::unique_ptr<gl::Program> shader;
    static struct wl_eglstream_controller_interface const impl;
};

class GLRenderingProvider : public graphics::GLRenderingProvider
{
public:
    GLRenderingProvider(EGLDisplay dpy, std::unique_ptr<renderer::gl::Context> ctx);
    ~GLRenderingProvider();

    auto as_texture(std::shared_ptr<Buffer> buffer) -> std::shared_ptr<gl::Texture> override;

    auto suitability_for_allocator(std::shared_ptr<GraphicBufferAllocator> const& target) -> probe::Result override;

    auto suitability_for_display(DisplaySink& sink) -> probe::Result override;

    auto make_framebuffer_provider(DisplaySink& sink) -> std::unique_ptr<FramebufferProvider> override;

    auto surface_for_sink(
        DisplaySink& sink,
        GLConfig const& gl_config) -> std::unique_ptr<gl::OutputSurface> override;
private:
    EGLDisplay dpy;
    std::shared_ptr<common::EGLContextExecutor> const egl_delegate;
    std::unique_ptr<renderer::gl::Context> const ctx;
};

}
}
}

#endif // MIR_PLATFORMS_EGLSTREAM_BUFFER_ALLOCATOR_
