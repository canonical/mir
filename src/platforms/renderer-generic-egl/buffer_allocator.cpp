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
#include "mir/renderer/gl/context_source.h"
#include "mir/graphics/egl_wayland_allocator.h"
#include "mir/executor.h"
#include "mir/renderer/gl/gl_surface.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/drm_formats.h"
#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <cassert>
#include <fcntl.h>

#include <wayland-server.h>

#define MIR_LOG_COMPONENT "gbm-kms-buffer-allocator"
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
    EGLint num_configs;
    
    if (eglChooseConfig(dpy, config_attr, &cfg, 1, &num_configs) != EGL_TRUE || num_configs != 1)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to find any matching EGL config")));
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

mge::BufferAllocator::BufferAllocator(EGLDisplay dpy, EGLContext share_with)
    : ctx{std::make_unique<SurfacelessEGLContext>(dpy, share_with)},
      egl_delegate{
          std::make_shared<mgc::EGLContextExecutor>(ctx->make_share_context())},
      egl_extensions(std::make_shared<mg::EGLExtensions>())
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

    return std::make_shared<mgc::MemoryBackedShmBuffer>(size, format, egl_delegate);
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
        mg::EGLExtensions::EXTImageDmaBufImportModifiers modifier_ext{dpy};
        dmabuf_extension =
            std::unique_ptr<LinuxDmaBufUnstable, std::function<void(LinuxDmaBufUnstable*)>>(
                new LinuxDmaBufUnstable{
                    display,
                    dpy,
                    egl_extensions,
                    modifier_ext,
                },
                [wayland_executor](LinuxDmaBufUnstable* global)
                {
                    // The global must be destroyed on the Wayland thread
                    wayland_executor->spawn(
                        [global]()
                        {
                            /* This is safe against double-frees, as the WaylandExecutor
                             * guarantees that work scheduled will only run while the Wayland
                             * event loop is running, and the main loop is stopped before
                             * wl_display_destroy() frees any globals
                             *
                             * This will, however, leak the global if the main loop is destroyed
                             * before the buffer allocator. Fixing that requires work in the
                             * wrapper generator.
                             */
                            delete global;
                        });
                });
        mir::log_info("Enabled linux-dmabuf import support");
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
    if (egl_display_bound)
    {
        auto context_guard = mir::raii::paired_calls(
            [this]() { ctx->make_current(); },
            [this]() { ctx->release_current(); });
        auto dpy = eglGetCurrentDisplay();

        mg::wayland::unbind_display(dpy, display, *egl_extensions);
    }
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
        egl_delegate,
        std::move(on_consumed),
        std::move(on_release));
}

auto mge::BufferAllocator::shared_egl_context() -> EGLContext
{
    return static_cast<EGLContext>(*ctx);
}

auto mge::GLRenderingProvider::as_texture(std::shared_ptr<Buffer> buffer) -> std::shared_ptr<gl::Texture>
{
    return std::dynamic_pointer_cast<gl::Texture>(buffer);
}

namespace
{
template<void (*allocator)(GLsizei, GLuint*), void (* deleter)(GLsizei, GLuint const*)>
class GLHandle
{
public:
    GLHandle()
    {
        (*allocator)(1, &id);
    }

    ~GLHandle()
    {
        if (id)
            (*deleter)(1, &id);
    }

    GLHandle(GLHandle const&) = delete;
    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from)
        : id{from.id}
    {
        from.id = 0;
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id;
};

using RenderbufferHandle = GLHandle<&glGenRenderbuffers, &glDeleteRenderbuffers>;
using FramebufferHandle = GLHandle<&glGenFramebuffers, &glDeleteFramebuffers>;


class CPUCopyOutputSurface : public mg::gl::OutputSurface
{
public:
    CPUCopyOutputSurface(
        EGLDisplay dpy,
        EGLContext ctx,
        std::shared_ptr<mg::DumbDisplayProvider> allocator,
        geom::Size size)
        : allocator{std::move(allocator)},
          dpy{dpy},
          ctx{ctx},
          size_{size}
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make share context current"));   
        }
        
        glBindRenderbuffer(GL_RENDERBUFFER, colour_buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, size_.width.as_int(), size_.height.as_int());

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colour_buffer);

        auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            switch (status)
            {
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    BOOST_THROW_EXCEPTION((
                        std::runtime_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"}
                        ));
                case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                    // Somehow we've managed to attach buffers with mismatched sizes?
                    BOOST_THROW_EXCEPTION((
                        std::logic_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS"}
                        ));
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    BOOST_THROW_EXCEPTION((
                        std::logic_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"}
                        ));
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    // This is the only one that isn't necessarily a programming error
                    BOOST_THROW_EXCEPTION((
                        std::runtime_error{"FBO is incomplete: formats selected are not supported by this GL driver"}
                        ));
                case 0:
                    BOOST_THROW_EXCEPTION((
                        mg::gl_error("Failed to verify GL Framebuffer completeness")));
            }
            BOOST_THROW_EXCEPTION((
                std::runtime_error{
                    std::string{"Unknown GL framebuffer error code: "} + std::to_string(status)}));
        }
    }

    void bind() override
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    void make_current() override
    {
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx);
    }

    auto commit() -> std::unique_ptr<mg::Framebuffer> override
    {
        auto fb = allocator->alloc_fb(size());
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        {
            auto mapping = fb->map_writeable();
            /*
             * TODO: This introduces a pipeline stall; GL must wait for all previous rendering commands
             * to complete before glReadPixels returns. We could instead do something fancy with
             * pixel buffer objects to defer this cost.
             */
            /*
             * TODO: We are assuming that the framebuffer pixel format is RGBX
             */
            glReadPixels(
                0, 0,
                size_.width.as_int(), size_.height.as_int(),
                GL_RGBA, GL_UNSIGNED_BYTE, mapping->data());
        }
        return fb;
    }

    auto size() const -> geom::Size override
    {
        return size_;
    }

    auto layout() const -> Layout override
    {
        return Layout::TopRowFirst;
    }

private:
    std::shared_ptr<mg::DumbDisplayProvider> const allocator;
    EGLDisplay const dpy;
    EGLContext const ctx;
    geom::Size const size_;
    RenderbufferHandle const colour_buffer;
    FramebufferHandle const fbo;
};

class EGLOutputSurface : public mg::gl::OutputSurface
{
public:
    EGLOutputSurface(
        std::unique_ptr<mg::GenericEGLDisplayProvider::EGLFramebuffer> fb)
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

    auto commit() -> std::unique_ptr<mg::Framebuffer> override
    {
        return fb->clone_handle();
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
    std::unique_ptr<mg::GenericEGLDisplayProvider::EGLFramebuffer> const fb;
};
}

auto mge::GLRenderingProvider::surface_for_output(
    std::shared_ptr<DisplayInterfaceProvider> framebuffer_provider,
    geometry::Size size,
    GLConfig const& config)
    -> std::unique_ptr<gl::OutputSurface>
{
    if (auto egl_display = framebuffer_provider->acquire_interface<GenericEGLDisplayProvider>())
    {
        return std::make_unique<EGLOutputSurface>(egl_display->alloc_framebuffer(config, ctx));
    }
    auto dumb_display = framebuffer_provider->acquire_interface<DumbDisplayProvider>();
    
    return std::make_unique<CPUCopyOutputSurface>(
        dpy,
        ctx,
        std::move(dumb_display),
        size);
}

auto mge::GLRenderingProvider::make_framebuffer_provider(std::shared_ptr<DisplayInterfaceProvider> /*target*/)
    -> std::unique_ptr<FramebufferProvider>
{
    // TODO: Work out under what circumstances the EGL renderer *can* provide overlayable framebuffers
    class NullFramebufferProvider : public FramebufferProvider
    {
    public:
        auto buffer_to_framebuffer(std::shared_ptr<Buffer>) -> std::unique_ptr<Framebuffer> override
        {
            // It is safe to return nullptr; this will be treated as “this buffer cannot be used as
            // a framebuffer”.
            return {};
        }
    };
    return std::make_unique<NullFramebufferProvider>();
}

mge::GLRenderingProvider::GLRenderingProvider(
    EGLDisplay dpy,
    EGLContext ctx)
    : dpy{dpy},
      ctx{ctx}
{
}
