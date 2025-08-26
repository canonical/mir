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

#include "buffer_allocator.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/linux_dmabuf.h"
#include "mir/anonymous_shm_file.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/graphics/platform.h"
#include "shm_buffer.h"
#include "mir/graphics/egl_context_executor.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/raii.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/egl_wayland_allocator.h"
#include "mir/executor.h"
#include "mir/renderer/gl/gl_surface.h"
#include "mir/graphics/display_sink.h"
#include "mir/graphics/drm_formats.h"
#include "mir/graphics/egl_error.h"
#include "cpu_copy_output_surface.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <drm_fourcc.h>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <cassert>
#include <fcntl.h>

#include <wayland-server.h>

#define MIR_LOG_COMPONENT "generic-egl-buffer-allocator"
#include <mir/log.h>
#include <mutex>

namespace mg  = mir::graphics;
namespace mge = mg::egl::generic;
namespace mgc = mg::common;
namespace geom = mir::geometry;

namespace
{
auto make_share_only_context(EGLDisplay dpy, std::optional<EGLContext> share_with) -> EGLContext
{
    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig cfg;
    if (!mg::has_egl_extension(dpy, "EGL_KHR_no_config_context"))
    {
        EGLint num_configs;

        if (eglChooseConfig(dpy, config_attr, &cfg, 1, &num_configs) != EGL_TRUE || num_configs != 1)
        {
            BOOST_THROW_EXCEPTION((mg::egl_error("Failed to find any matching EGL config")));
        }
    }
    else
    {
        cfg = EGL_NO_CONFIG_KHR;
    }

    auto ctx = eglCreateContext(dpy, cfg, share_with.value_or(EGL_NO_CONTEXT), context_attr);
    if (ctx == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }
    return ctx;
}

class SurfacelessEGLContext : public mir::renderer::gl::Context
{
public:
    explicit SurfacelessEGLContext(EGLDisplay dpy)
        : dpy{dpy},
          ctx{make_share_only_context(dpy, {})}
    {
    }

    SurfacelessEGLContext(EGLDisplay dpy, EGLContext share_with)
        : dpy{dpy},
          ctx{make_share_only_context(dpy, share_with)}
    {
    }

    ~SurfacelessEGLContext() override
    {
        make_current();
        release_current();
        eglDestroyContext(dpy, ctx);
    }

    void make_current() const override
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
        }
    }

    void release_current() const override
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release current EGL context"));
        }
    }

    auto make_share_context() const -> std::unique_ptr<Context> override
    {
        return std::unique_ptr<Context>{new SurfacelessEGLContext{dpy, ctx}};
    }

    explicit operator EGLContext() override
    {
        return ctx;
    }
private:
    EGLDisplay const dpy;
    EGLContext const ctx;
};
}

mge::BufferAllocator::BufferAllocator(
    EGLDisplay dpy,
    EGLContext share_with,
    std::shared_ptr<DMABufEGLProvider> dmabuf_provider)
    : ctx{std::make_unique<SurfacelessEGLContext>(dpy, share_with)},
      egl_delegate{
          std::make_shared<mgc::EGLContextExecutor>(ctx->make_share_context())},
      egl_extensions(std::make_shared<mg::EGLExtensions>()),
      dmabuf_provider{std::move(dmabuf_provider)}
{
}

mge::BufferAllocator::~BufferAllocator() = default;

std::shared_ptr<mg::Buffer> mge::BufferAllocator::alloc_software_buffer(
    geom::Size size, MirPixelFormat format)
{
    if (!mgc::MemoryBackedShmBuffer::supports(format))
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Trying to create SHM buffer with unsupported pixel format"));
    }

    return std::make_shared<mgc::MemoryBackedShmBuffer>(size, format);
}

std::vector<MirPixelFormat> mge::BufferAllocator::supported_pixel_formats()
{
    /*
     * supported_pixel_formats() is kind of a kludge. The right answer depends
     * on whether you're using hardware or software, and it depends on
     * the usage type (e.g. scanout). In the future it's also expected to
     * depend on the GPU model in use at runtime.
     *   To be precise, ShmBuffer now supports OpenGL compositing of all
     * but one MirPixelFormat (bgr_888). But GBM only supports [AX]RGB.
     * So since we don't yet have an adequate API in place to query what the
     * intended usage will be, we need to be conservative and report the
     * intersection of ShmBuffer and GBM's pixel format support. That is
     * just these two. Be aware however you can create a software surface
     * with almost any pixel format and it will also work...
     *   TODO: Convert this to a loop that just queries the intersection of
     * gbm_device_is_format_supported and ShmBuffer::supports(), however not
     * yet while the former is buggy. (FIXME: LP: #1473901)
     */
    static std::vector<MirPixelFormat> const pixel_formats{
        mir_pixel_format_argb_8888,
        mir_pixel_format_xrgb_8888
    };

    return pixel_formats;
}

void mge::BufferAllocator::bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    try
    {
        mg::wayland::bind_display(dpy, display, *egl_extensions);
        egl_display_bound = true;
    }
    catch (...)
    {
        log(
            logging::Severity::warning,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "Failed to bind EGL Display to Wayland display, falling back to software buffers");
    }

    try
    {
        if (dmabuf_provider)
        {
            dmabuf_extension = std::make_unique<LinuxDmaBuf>(display, dmabuf_provider);
            mir::log_info("Enabled linux-dmabuf import support");
        }
    }
    catch (std::runtime_error const& error)
    {
        mir::log_info(
            "Cannot enable linux-dmabuf import support: %s", error.what());
        mir::log(
            mir::logging::Severity::debug,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "Detailed error: ");
    }

    this->wayland_executor = std::move(wayland_executor);
}

void mge::BufferAllocator::unbind_display(wl_display* display)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    if (egl_display_bound)
    {
        mg::wayland::unbind_display(dpy, display, *egl_extensions);
    }
    dmabuf_extension.reset();
}

std::shared_ptr<mg::Buffer> mge::BufferAllocator::buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });

    if (auto dmabuf = dmabuf_extension->buffer_from_resource(
        buffer,
        std::function<void()>{on_consumed},
        std::function<void()>{on_release},
        egl_delegate))
    {
        return dmabuf;
    }
    return mg::wayland::buffer_from_resource(
        buffer,
        std::move(on_consumed),
        std::move(on_release),
        *egl_extensions,
        egl_delegate);
}

auto mge::BufferAllocator::buffer_from_shm(
    std::shared_ptr<renderer::software::RWMappableBuffer> data,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release) -> std::shared_ptr<Buffer>
{
    return std::make_shared<mgc::NotifyingMappableBackedShmBuffer>(
        std::move(data),
        std::move(on_consumed),
        std::move(on_release));
}

auto mge::BufferAllocator::shared_egl_context() -> EGLContext
{
    return static_cast<EGLContext>(*ctx);
}

auto mge::GLRenderingProvider::as_texture(std::shared_ptr<Buffer> buffer) -> std::shared_ptr<gl::Texture>
{
    std::shared_ptr<NativeBufferBase> native_buffer{buffer, buffer->native_buffer_base()};
    if (dmabuf_provider)
    {
        if (auto tex = dmabuf_provider->as_texture(native_buffer))
        {
            return tex;
        }
    }
    if (auto shm = std::dynamic_pointer_cast<mgc::ShmBuffer>(native_buffer))
    {
        return shm->texture_for_provider(egl_delegate, this);
    }

    // TODO: Should this be abstracted, like dmabuf_provider above?
    return std::dynamic_pointer_cast<gl::Texture>(native_buffer);
}

namespace
{

class FramebufferAdapter : public mg::Buffer, public mg::NativeBufferBase
{
    using EGLFramebuffer = mir::graphics::GenericEGLDisplayAllocator::EGLFramebuffer;

public:
    FramebufferAdapter(std::unique_ptr<EGLFramebuffer> framebuffer) :
        framebuffer{std::move(framebuffer)}
    {
    }

    mg::BufferID id() const override
    {
        return mg::BufferID{static_cast<uint32_t>(reinterpret_cast<uintptr_t>(framebuffer.get()))};
    }

    geom::Size size() const override
    {
        return framebuffer->size();
    }

    MirPixelFormat pixel_format() const override
    {
        return framebuffer->format();
    }

    std::unique_ptr<EGLFramebuffer> framebuffer;
};
class EGLOutputSurface : public mg::gl::OutputSurface
{
public:
    EGLOutputSurface(
        std::unique_ptr<mg::GenericEGLDisplayAllocator::EGLFramebuffer> fb)
        : fb{std::move(fb)}
    {
    }

    void bind() override
    {
    }

    void make_current() override
    {
        fb->make_current();
    }

    void release_current() override
    {
        fb->release_current();
    }

    auto commit() -> std::unique_ptr<mg::Buffer> override
    {
        return std::make_unique<FramebufferAdapter>(fb->clone_handle());
    }

    auto size() const -> geom::Size override
    {
        return fb->size();
    }

    auto layout() const -> Layout override
    {
        return Layout::GL;
    }

private:
    std::unique_ptr<mg::GenericEGLDisplayAllocator::EGLFramebuffer> const fb;
};
}

auto mge::GLRenderingProvider::suitability_for_allocator(std::shared_ptr<GraphicBufferAllocator> const& target)
    -> probe::Result
{
    // TODO: We *can* import from other allocators, maybe (anything with dma-buf is probably possible)
    // For now, the simplest thing is to bind hard to own own allocator.
    if (dynamic_cast<mge::BufferAllocator*>(target.get()))
    {
        return probe::best;
    }
    return probe::unsupported;
}

auto mge::GLRenderingProvider::suitability_for_display(
    DisplaySink& sink) -> probe::Result
{
    if (sink.acquire_compatible_allocator<GenericEGLDisplayAllocator>())
    {
        /* We're effectively nested on an underlying EGL platform.
         *
         * We'll work fine, but if there's a hardware-specific platform
         * let it take over.
         */
        return probe::nested;
    }

    if (sink.acquire_compatible_allocator<CPUAddressableDisplayAllocator>())
    {
        /* We can *work* on a CPU-backed surface, but if anything's better
         * we should use something else!
         */
        return probe::supported;
    }

    return probe::unsupported;
}

auto mge::GLRenderingProvider::surface_for_sink(
    DisplaySink& sink,
    GLConfig const& config)
    -> std::unique_ptr<gl::OutputSurface>
{
    if (auto egl_display = sink.acquire_compatible_allocator<GenericEGLDisplayAllocator>())
    {
        return std::make_unique<EGLOutputSurface>(egl_display->alloc_framebuffer(config, ctx));
    }
    auto cpu_provider = sink.acquire_compatible_allocator<CPUAddressableDisplayAllocator>();

    return std::make_unique<mgc::CPUCopyOutputSurface>(
        dpy,
        ctx,
        *cpu_provider,
        config);
}

auto mge::GLRenderingProvider::make_framebuffer_provider(DisplaySink& /*sink*/)
    -> std::unique_ptr<FramebufferProvider>
{
    // TODO: Work out under what circumstances the EGL renderer *can* provide overlayable framebuffers
    class NullFramebufferProvider : public FramebufferProvider
    {
    public:
        auto buffer_to_framebuffer(std::shared_ptr<Buffer> buf) -> std::unique_ptr<Framebuffer> override
        {
            // It is safe to return nullptr; this will be treated as “this buffer cannot be used as
            // a framebuffer”.
            if (auto fb = std::dynamic_pointer_cast<FramebufferAdapter>(buf))
            {
                return std::move(fb->framebuffer);
            }
            return {};
        }
    };
    return std::make_unique<NullFramebufferProvider>();
}

mge::GLRenderingProvider::GLRenderingProvider(
    EGLDisplay dpy,
    EGLContext ctx,
    std::shared_ptr<mg::DMABufEGLProvider> dmabuf_provider,
    std::shared_ptr<mgc::EGLContextExecutor> egl_delegate)
    : dpy{dpy},
      ctx{ctx},
      dmabuf_provider{std::move(dmabuf_provider)},
      egl_delegate(egl_delegate)
{
}
